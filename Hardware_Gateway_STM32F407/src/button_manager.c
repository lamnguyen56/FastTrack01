/**
 * @file button_manager.c
 * @brief Cài đặt bộ quét cụm 3 nút bấm với máy trạng thái phân biệt Single / Double Click
 *        và rào chắn chống nhấn đúp thừa (click 3, 4 lần)
 * @description
 *  - NÚT 1 (PE0 / PA0 Onboard): PLAY / PAUSE
 *      + Nhấn 1 lần: Đảo trạng thái Play <-> Pause.
 *  - NÚT 2 (PE1): NEXT / PREVIOUS
 *      + Nhấn 1 lần: NEXT TRACK (0x03)
 *      + Nhấn 2 lần: PREVIOUS TRACK (0x04)
 *      + Khóa chống spam: Bỏ qua hoàn toàn các lần nhấn thứ 3, 4
 *  - NÚT 3 (PE2): TUA NHANH +10S / -10S
 *      + Nhấn 1 lần: Tua tiến +10s (Fast Forward: SeekCmd=0x05, Step=10)
 *      + Nhấn 2 lần: Tua lùi -10s  (Fast Backward: SeekCmd=0x06, Step=10)
 *      + Khóa chống spam: Bỏ qua hoàn toàn các lần nhấn thứ 3, 4
 */

#include "button_manager.h"
#include "led_indicator.h"

static ButtonCommandCallback_t s_cmdCallback = NULL;
static PlaybackState_t         s_currentPlaybackState = PLAYBACK_STATE_STOPPED;
static bool                    s_localPlayState = false; /* Cờ dự phòng đảo Play/Pause nếu chưa có Telemetry */

/* ============================================================================
 * ĐỊNH NGHĨA MÁY TRẠNG THÁI (FINITE STATE MACHINE - FSM) CHO NÚT MULTI-CLICK
 * ============================================================================ */
typedef enum {
    BTN_FSM_IDLE = 0,               /* Trạng thái nghỉ, sẵn sàng nhận chuỗi bấm mới */
    BTN_FSM_PRESSED_1,              /* Đang ấn lần 1 (sau khi qua lọc rung 50ms) */
    BTN_FSM_RELEASED_WAIT_2,        /* Đã nhả lần 1, đang đợi cú click thứ 2 trong 350ms */
    BTN_FSM_PRESSED_2,              /* Đang ấn lần 2 (đã phát hiện Double Click thành công) */
    BTN_FSM_LOCKOUT                 /* Khóa bảo vệ: Chặn click 3, 4 và chờ nút nhả yên tĩnh */
} BtnFsmState_t;

/* Cấu trúc dữ liệu theo dõi nút bấm */
typedef struct {
    uint16_t      pin;
    bool          lastRaw;
    bool          stable;
    uint32_t      lastDebounceTime;
    BtnFsmState_t fsmState;
    uint32_t      pressStartTime;
    uint32_t      windowStartTime;
    uint32_t      releaseTime;
} MultiClickButton_t;

/* Khởi tạo trạng thái cho Nút 2 và Nút 3 */
static MultiClickButton_t s_btnNextPrev = {BTN_PIN_NEXT_PREV, false, false, 0, BTN_FSM_IDLE, 0, 0, 0};
static MultiClickButton_t s_btnSeek     = {BTN_PIN_SEEK,      false, false, 0, BTN_FSM_IDLE, 0, 0, 0};

/* Quản lý nút 1 (Play/Pause) */
static bool     s_btn1LastRaw = false;
static bool     s_btn1Stable  = false;
static uint32_t s_btn1DebounceTime = 0;
static uint32_t s_btn1LastFireTime = 0;

/* Quản lý nút User PA0 Onboard */
static bool     s_userBtnLastRaw = false;
static bool     s_userBtnStable  = false;
static uint32_t s_userBtnDebounceTime = 0;
static uint32_t s_userBtnLastFireTime = 0;

void Button_Init(void) {
    // 1. Khởi tạo chân nút bấm Onboard PA0 (Active HIGH)
    BTN_USER_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BTN_USER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Board Discovery đã có sẵn trở pull-down ngoài
    HAL_GPIO_Init(BTN_USER_PORT, &GPIO_InitStruct);

    // 2. Khởi tạo cụm 3 nút bấm ngoại vi (Port E - Active LOW với Pull-up nội)
    BTN_CLUSTER_CLK_ENABLE();
    GPIO_InitStruct.Pin = BTN_PIN_PLAY_PAUSE | BTN_PIN_NEXT_PREV | BTN_PIN_SEEK;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_CLUSTER_PORT, &GPIO_InitStruct);
}

void Button_RegisterCallback(ButtonCommandCallback_t cb) {
    s_cmdCallback = cb;
}

void Button_UpdatePlaybackState(PlaybackState_t state) {
    s_currentPlaybackState = state;
    s_localPlayState = (state == PLAYBACK_STATE_PLAYING);
}

/**
 * @brief Kích hoạt lệnh Play/Pause
 */
static void TriggerPlayPause(void) {
    uint8_t targetCmd;
    if (s_currentPlaybackState == PLAYBACK_STATE_PLAYING || s_localPlayState) {
        targetCmd = CAN_CMD_PAUSE;
        s_localPlayState = false;
    } else {
        targetCmd = CAN_CMD_PLAY;
        s_localPlayState = true;
    }

    if (s_cmdCallback) {
        s_cmdCallback(targetCmd, CAN_SEEK_NONE, 0);
    }
}

/**
 * @brief Bộ điều phối máy trạng thái chung cho các nút hỗ trợ Single / Double Click
 * @param btn Con trỏ quản lý nút
 * @param currentTickMs Thời gian thực hiện tại
 * @param isRawActive Trạng thái chân GPIO đang đọc (true: đang ấn)
 * @param onSingleClick Con trỏ hàm thực thi khi xác nhận Single Click
 * @param onDoubleClick Con trỏ hàm thực thi khi xác nhận Double Click
 */
static void ProcessMultiClickFsm(MultiClickButton_t *btn, uint32_t currentTickMs, bool isRawActive,
                                void (*onSingleClick)(void), void (*onDoubleClick)(void)) {
    // 1. Lọc chống rung phím cơ học (Debounce 50ms)
    if (isRawActive != btn->lastRaw) {
        btn->lastDebounceTime = currentTickMs;
        btn->lastRaw = isRawActive;
    }

    bool justPressed = false;
    bool justReleased = false;

    if ((currentTickMs - btn->lastDebounceTime) >= BUTTON_DEBOUNCE_MS) {
        if (isRawActive != btn->stable) {
            btn->stable = isRawActive;
            if (btn->stable) {
                justPressed = true;
            } else {
                justReleased = true;
            }
        }
    }

    // 2. Máy trạng thái (FSM) xử lý Single Click / Double Click & Khóa chống spam
    switch (btn->fsmState) {
        case BTN_FSM_IDLE:
            if (justPressed) {
                btn->pressStartTime = currentTickMs;
                btn->fsmState = BTN_FSM_PRESSED_1;
            }
            break;

        case BTN_FSM_PRESSED_1:
            if (justReleased) {
                uint32_t holdDuration = currentTickMs - btn->pressStartTime;
                // Nếu người dùng giữ phím lâu (> 500ms), nhả ra là gửi Single Click ngay, không chờ Double
                if (holdDuration >= BUTTON_LONG_HOLD_THRESHOLD) {
                    if (onSingleClick) onSingleClick();
                    LED_TriggerTxBlink();
                    btn->releaseTime = currentTickMs;
                    btn->fsmState = BTN_FSM_LOCKOUT;
                } else {
                    // Cú nhấn nhanh: chuyển sang trạng thái chờ cú click thứ 2 trong 350ms
                    btn->windowStartTime = currentTickMs;
                    btn->fsmState = BTN_FSM_RELEASED_WAIT_2;
                }
            }
            break;

        case BTN_FSM_RELEASED_WAIT_2:
            if (justPressed) {
                // PHÁT HIỆN CÚ CLICK THỨ 2 TRONG CỬA SỔ THỜI GIAN -> XÁC NHẬN DOUBLE CLICK!
                if (onDoubleClick) onDoubleClick();
                LED_TriggerTxDoubleBlink();
                btn->fsmState = BTN_FSM_PRESSED_2;
            } else if ((currentTickMs - btn->windowStartTime) >= BUTTON_DOUBLE_CLICK_MS) {
                // HẾT CỬA SỔ CHỜ MÀ KHÔNG CÓ CLICK THỨ 2 -> XÁC NHẬN SINGLE CLICK!
                if (onSingleClick) onSingleClick();
                LED_TriggerTxBlink();
                btn->releaseTime = currentTickMs;
                btn->fsmState = BTN_FSM_LOCKOUT;
            }
            break;

        case BTN_FSM_PRESSED_2:
            // Đã kích hoạt Double Click lúc vừa ấn xuống, chờ nhả phím để vào Lockout
            if (justReleased) {
                btn->releaseTime = currentTickMs;
                btn->fsmState = BTN_FSM_LOCKOUT;
            }
            break;

        case BTN_FSM_LOCKOUT:
            // RÀO CHẶN CHỐNG SPAM (ANTI-SPAM GUARD):
            // Trong trạng thái này, mọi cú click thứ 3, 4, 5 đều bị bỏ qua hoàn toàn.
            // Chỉ khi phím được thả ra và giữ yên tĩnh đủ BUTTON_COOLDOWN_MS (200ms) thì mới quay về IDLE.
            if (btn->stable) {
                // Nếu người dùng vẫn đang đè nút hoặc nhấp nháy tiếp, reset lại timer cooldown
                btn->releaseTime = currentTickMs;
            } else {
                if ((currentTickMs - btn->releaseTime) >= BUTTON_COOLDOWN_MS) {
                    btn->fsmState = BTN_FSM_IDLE;
                }
            }
            break;
    }
}

/* Các hàm Callback cho Nút 2: Next / Prev */
static void OnBtn2SingleClick(void) {
    if (s_cmdCallback) {
        s_cmdCallback(CAN_CMD_NEXT, CAN_SEEK_NONE, 0);
    }
}

static void OnBtn2DoubleClick(void) {
    if (s_cmdCallback) {
        s_cmdCallback(CAN_CMD_PREVIOUS, CAN_SEEK_NONE, 0);
    }
}

/* Các hàm Callback cho Nút 3: Tua +10s / Tua -10s */
static void OnBtn3SingleClick(void) {
    if (s_cmdCallback) {
        s_cmdCallback(CAN_CMD_IDLE, CAN_SEEK_FAST_FORWARD, 10);
    }
}

static void OnBtn3DoubleClick(void) {
    if (s_cmdCallback) {
        s_cmdCallback(CAN_CMD_IDLE, CAN_SEEK_FAST_BACKWARD, 10);
    }
}

void Button_Scan(uint32_t currentTickMs) {
    bool anyButtonPressed = false;

    /* =========================================================================
     * 1. NÚT 1: PLAY / PAUSE (PE0 & PA0 Onboard)
     * ========================================================================= */
    // A. Chân PE0 (Active LOW)
    bool rawB1 = (HAL_GPIO_ReadPin(BTN_CLUSTER_PORT, BTN_PIN_PLAY_PAUSE) == GPIO_PIN_RESET);
    if (rawB1 != s_btn1LastRaw) {
        s_btn1DebounceTime = currentTickMs;
        s_btn1LastRaw = rawB1;
    }
    if ((currentTickMs - s_btn1DebounceTime) >= BUTTON_DEBOUNCE_MS) {
        if (rawB1 != s_btn1Stable) {
            s_btn1Stable = rawB1;
            if (s_btn1Stable) { // Bắt sườn ấn xuống
                if ((currentTickMs - s_btn1LastFireTime) >= BUTTON_COOLDOWN_MS) {
                    s_btn1LastFireTime = currentTickMs;
                    TriggerPlayPause();
                }
            }
        }
    }
    if (s_btn1Stable) anyButtonPressed = true;

    // B. Nút User PA0 Onboard (Active HIGH)
    bool rawUser = (HAL_GPIO_ReadPin(BTN_USER_PORT, BTN_USER_PIN) == BTN_USER_ACTIVE_STATE);
    if (rawUser != s_userBtnLastRaw) {
        s_userBtnDebounceTime = currentTickMs;
        s_userBtnLastRaw = rawUser;
    }
    if ((currentTickMs - s_userBtnDebounceTime) >= BUTTON_DEBOUNCE_MS) {
        if (rawUser != s_userBtnStable) {
            s_userBtnStable = rawUser;
            if (s_userBtnStable) { // Bắt sườn ấn xuống
                if ((currentTickMs - s_userBtnLastFireTime) >= BUTTON_COOLDOWN_MS) {
                    s_userBtnLastFireTime = currentTickMs;
                    TriggerPlayPause();
                }
            }
        }
    }
    if (s_userBtnStable) anyButtonPressed = true;

    /* =========================================================================
     * 2. NÚT 2: NEXT / PREVIOUS (PE1 - Active LOW)
     * - Ấn 1 lần: NEXT TRACK
     * - Ấn 2 lần: PREVIOUS TRACK
     * - Rào chắn: Khóa chặn lần ấn 3, 4
     * ========================================================================= */
    bool rawNextPrev = (HAL_GPIO_ReadPin(BTN_CLUSTER_PORT, s_btnNextPrev.pin) == GPIO_PIN_RESET);
    ProcessMultiClickFsm(&s_btnNextPrev, currentTickMs, rawNextPrev, OnBtn2SingleClick, OnBtn2DoubleClick);
    if (s_btnNextPrev.stable) anyButtonPressed = true;

    /* =========================================================================
     * 3. NÚT 3: TUA NHANH +10S / -10S (PE2 - Active LOW)
     * - Ấn 1 lần: TUA TIẾN +10S
     * - Ấn 2 lần: TUA LÙI -10S
     * - Rào chắn: Khóa chặn lần ấn 3, 4
     * ========================================================================= */
    bool rawSeek = (HAL_GPIO_ReadPin(BTN_CLUSTER_PORT, s_btnSeek.pin) == GPIO_PIN_RESET);
    ProcessMultiClickFsm(&s_btnSeek, currentTickMs, rawSeek, OnBtn3SingleClick, OnBtn3DoubleClick);
    if (s_btnSeek.stable) anyButtonPressed = true;

    /* =========================================================================
     * ĐIỀU KHIỂN LED STATUS (Bật sáng LED Cam LD3 khi có bất kỳ nút nào đang ấn)
     * ========================================================================= */
    LED_SetButtonPressed(anyButtonPressed);
}
