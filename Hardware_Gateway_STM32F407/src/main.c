/**
 * @file main.c
 * @brief Chương trình chính cho STM32F407VGT6 Automotive CAN Gateway
 * @project FastTrack01 - USB Video Player Controlled via CAN
 * @board STM32F4DISCOVERY / Generic STM32F407VGT6 Core Board
 * @description
 *  - Quản lý truyền frame CAN ID 0x100 đến Raspberry Pi khi nhấn nút bấm.
 *  - Phản hồi trạng thái ngay lập tức qua cụm LED:
 *      + LED Orange (LD3): Sáng khi có nút đang được nhấn (Phản hồi xúc giác/thị giác).
 *      + LED Green (LD4): Chớp sáng tức thì khi frame CAN được đẩy thành công lên bus.
 *      + LED Red (LD5): Bật sáng nếu xảy ra lỗi CAN Bus (Bus-off, mất trở đầu cuối, disconnected).
 *      + LED Blue (LD6): Sáng liên tục khi Raspberry Pi đang ở trạng thái PLAYING video.
 */

#include "main.h"

/* Khai báo hàm nguyên mẫu nội bộ */
static void OnButtonCommandTriggered(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal);
static void OnStatusPacketReceived(const CanStatusPacket_t *pStatus);

int main(void) {
    /* 1. Khởi tạo thư viện HAL và bộ đếm thời gian hệ thống SysTick */
    HAL_Init();

    /* 2. Cấu hình xung nhịp Clock hệ thống: SYSCLK = 168MHz, APB1 = 42MHz */
    SystemClock_Config();

    /* 3. Khởi tạo cụm 4 LED trạng thái trên bo mạch */
    LED_Init();

    /* 4. Khởi tạo ngoại vi đọc nút bấm (PA0 User Button & SWC Pins) */
    Button_Init();

    /* 5. Khởi tạo ngoại vi bxCAN (CAN1) 500 kbps, cấu hình bộ lọc */
    if (!CAN_Driver_Init()) {
        LED_SetError(true);
        // Có thể tiếp tục chạy vòng lặp để theo dõi lỗi
    }

    /* 6. Liên kết các sự kiện Callback giữa Nút bấm và CAN Driver:
     * - Khi nút bấm phát sinh lệnh -> Gọi hàm phát frame CAN 0x100
     * - Khi CAN nhận frame trạng thái 0x101 từ Pi -> Cập nhật sang nút bấm và LED
     */
    Button_RegisterCallback(OnButtonCommandTriggered);
    CAN_Driver_RegisterStatusCallback(OnStatusPacketReceived);

    /* 7. Vòng lặp chính (Super Loop) non-blocking */
    uint32_t lastHealthCheck = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        // Quét nút bấm với bộ lọc chống rung phím 50ms
        Button_Scan(now);

        // Cập nhật hiệu ứng chớp tắt của các LED trạng thái
        LED_Update(now);

        // Định kỳ kiểm tra tình trạng bus CAN mỗi 250ms
        if ((now - lastHealthCheck) >= 250) {
            lastHealthCheck = now;
            CAN_Driver_CheckBusHealth();
        }

        // Tạm dừng 2ms để tiết kiệm năng lượng
        HAL_Delay(2);
    }
}

/**
 * @brief Callback được gọi khi nút bấm vật lý kích hoạt lệnh điều khiển
 */
static void OnButtonCommandTriggered(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal) {
    // Truyền lệnh CAN ID 0x100
    CAN_Driver_SendCommand(cmdId, seekCmd, seekVal);
}

/**
 * @brief Callback được gọi khi nhận gói Telemetry ID 0x101 từ Raspberry Pi Target
 */
static void OnStatusPacketReceived(const CanStatusPacket_t *pStatus) {
    if (!pStatus) return;

    // Cập nhật trạng thái phát hiện tại cho nút bấm để tự động đảo trạng thái Play <-> Pause
    Button_UpdatePlaybackState((PlaybackState_t)pStatus->playbackState);
}

/**
 * @brief Cấu hình Clock hệ thống sử dụng thạch anh ngoài 8MHz (HSE)
 *        SYSCLK = 168 MHz, HCLK = 168 MHz, APB1 = 42 MHz, APB2 = 84 MHz
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Bật bộ điều chỉnh điện áp chính để chạy max 168 MHz
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Cấu hình Oscillator: Sử dụng HSE 8MHz kết hợp PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Cấu hình Clock cho các bus hệ thống
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; // 168MHz / 4 = 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; // 168MHz / 2 = 84MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Xử lý lỗi hệ thống nghiêm trọng
 */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        // Nháy LED đỏ báo lỗi hệ thống
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN_RED);
        for (volatile int i = 0; i < 500000; i++);
    }
}
