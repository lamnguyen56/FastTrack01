/**
 * ============================================================================
 * ESP32 CAN GATEWAY WEB DASHBOARD - CLIENT JAVASCRIPT LOGIC
 * Supports: Real-time WebSocket Protocol & Offline PC Mock Simulation Mode
 * ============================================================================
 */

let ws = null;
let isPlaying = false;
let mockMode = false;
let mockTimer = null;
let mockCurTime = 25;
let mockTotalTime = 240;
let mockAlive = 0;

// DOM Elements
const wsStatus = document.getElementById('wsStatus');
const btnMockToggle = document.getElementById('btnMockToggle');
const playbackStateBadge = document.getElementById('playbackStateBadge');
const usbStateBadge = document.getElementById('usbStateBadge');
const trackIndexText = document.getElementById('trackIndexText');
const heartbeatTag = document.getElementById('heartbeatTag');
const curTimeText = document.getElementById('curTimeText');
const totalTimeText = document.getElementById('totalTimeText');
const progressBarFill = document.getElementById('progressBarFill');
const progressPctText = document.getElementById('progressPctText');
const seekSlider = document.getElementById('seekSlider');
const playPauseIcon = document.getElementById('playPauseIcon');
const playPauseText = document.getElementById('playPauseText');
const snifferConsole = document.getElementById('snifferConsole');

// Format seconds into MM:SS
function formatTime(sec) {
  if (isNaN(sec) || sec < 0) sec = 0;
  const m = Math.floor(sec / 60);
  const s = Math.floor(sec % 60);
  return (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
}

// Add line to Live CAN sniffer console
function addCanLog(text, type = 'info') {
  const line = document.createElement('div');
  line.className = `log-line log-${type}`;
  
  const now = new Date();
  const timeStr = `[${now.toTimeString().split(' ')[0]}.${String(now.getMilliseconds()).padStart(3, '0')}]`;
  line.textContent = `${timeStr} ${text}`;
  
  snifferConsole.appendChild(line);
  // Cap at 150 log items
  while (snifferConsole.childNodes.length > 150) {
    snifferConsole.removeChild(snifferConsole.firstChild);
  }
  snifferConsole.scrollTop = snifferConsole.scrollHeight;
}

function clearCanLog() {
  snifferConsole.innerHTML = '';
  addCanLog('Console log đã được làm sạch.', 'info');
}

// --- WEBSOCKET CLIENT ---
function initWebSocket() {
  // If loaded as file:// on PC without ESP32, automatically enable mock mode
  if (window.location.protocol === 'file:') {
    enableMockMode(true);
    addCanLog('Phát hiện chạy offline từ file://. Tự động bật MOCK SIMULATION MODE.', 'info');
    return;
  }

  const wsUrl = `ws://${window.location.hostname || '192.168.4.1'}/ws`;
  addCanLog(`Đang kết nối WebSocket tới ${wsUrl}...`, 'info');

  try {
    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
      wsStatus.textContent = 'WS: CONNECTED';
      wsStatus.className = 'badge badge-connected';
      addCanLog('WebSocket đã kết nối thành công tới ESP32!', 'rx');
    };

    ws.onclose = () => {
      wsStatus.textContent = 'WS: DISCONNECTED';
      wsStatus.className = 'badge badge-disconnected';
      addCanLog('Mất kết nối WebSocket. Thử kết nối lại sau 2.5s...', 'err');
      if (!mockMode) {
        setTimeout(initWebSocket, 2500);
      }
    };

    ws.onerror = (err) => {
      console.warn('Lỗi kết nối WebSocket:', err);
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        if (data.type === 'telemetry') {
          handleTelemetryUpdate(data);
        } else if (data.type === 'log') {
          addCanLog(data.msg, data.isTx ? 'tx' : 'rx');
        }
      } catch (e) {
        console.error('Không thể parse JSON từ server:', event.data);
      }
    };
  } catch (err) {
    addCanLog(`Không thể mở WebSocket: ${err.message}`, 'err');
  }
}

// Handle CAN 0x101 Telemetry JSON from ESP32
function handleTelemetryUpdate(data) {
  // 1. Alive heartbeat
  heartbeatTag.textContent = `❤️ Alive: ${data.alive}`;

  // 2. Track Index
  trackIndexText.textContent = `#${String(data.track).padStart(2, '0')}`;

  // 3. Time & Progress
  curTimeText.textContent = formatTime(data.curTime);
  totalTimeText.textContent = formatTime(data.totalTime);

  const pct = data.totalTime > 0 ? ((data.curTime / data.totalTime) * 100).toFixed(1) : 0;
  progressBarFill.style.width = `${pct}%`;
  progressPctText.textContent = `${pct}%`;
  seekSlider.value = pct;

  // 4. Playback State
  // 0: Stopped, 1: Playing, 2: Paused, 3: Error
  playbackStateBadge.className = 'state-badge';
  if (data.state === 1) {
    isPlaying = true;
    playbackStateBadge.textContent = 'PLAYING';
    playbackStateBadge.classList.add('state-playing');
    playPauseIcon.textContent = '⏸';
    playPauseText.textContent = 'PAUSE';
  } else if (data.state === 2) {
    isPlaying = false;
    playbackStateBadge.textContent = 'PAUSED';
    playbackStateBadge.classList.add('state-paused');
    playPauseIcon.textContent = '▶';
    playPauseText.textContent = 'PLAY';
  } else if (data.state === 3) {
    isPlaying = false;
    playbackStateBadge.textContent = 'ERROR';
    playbackStateBadge.classList.add('state-error');
    playPauseIcon.textContent = '▶';
    playPauseText.textContent = 'PLAY';
  } else {
    isPlaying = false;
    playbackStateBadge.textContent = 'STOPPED';
    playbackStateBadge.classList.add('state-stopped');
    playPauseIcon.textContent = '▶';
    playPauseText.textContent = 'PLAY';
  }

  // 5. USB Health State (0: No USB, 1: Scanning, 2: Ready)
  usbStateBadge.className = 'state-badge';
  if (data.usb === 2) {
    usbStateBadge.textContent = 'READY (Mounted)';
    usbStateBadge.classList.add('usb-ready');
  } else if (data.usb === 1) {
    usbStateBadge.textContent = 'SCANNING...';
    usbStateBadge.classList.add('usb-scanning');
  } else {
    usbStateBadge.textContent = 'DISCONNECTED';
    usbStateBadge.classList.add('usb-disconnected');
  }
}

// --- SEND CAN COMMAND (ID 0x100) ---
function sendCanCommand(cmdId, seekCmd, seekVal) {
  if (mockMode) {
    handleMockCommand(cmdId, seekCmd, seekVal);
    return;
  }

  if (ws && ws.readyState === WebSocket.OPEN) {
    const payload = {
      action: 'cmd',
      cmd: cmdId,
      seek: seekCmd,
      val: seekVal
    };
    ws.send(JSON.stringify(payload));
  } else {
    addCanLog('Không thể gửi lệnh: WebSocket chưa sẵn sàng!', 'err');
  }
}

function togglePlayPause() {
  if (isPlaying) {
    sendCanCommand(2, 0, 0); // Lệnh Pause (0x02)
  } else {
    sendCanCommand(1, 0, 0); // Lệnh Play (0x01)
  }
}

function onSeekSliderChange(value) {
  // Convert percentage value to seek command
  addCanLog(`Kéo Slider tới mốc ${value}%. Gửi lệnh Seek tương ứng...`, 'tx');
  sendCanCommand(0, 5, 5); // Tạm thời gửi bước nhảy +5s hoặc theo logic target
}

// --- AUTOMATED SMOKE TEST ROUTINE ---
function runAutomatedSmokeTest() {
  const btn = document.getElementById('btnAutoTest');
  btn.disabled = true;
  btn.textContent = '⏳ ĐANG CHẠY KỊCH BẢN KIỂM THỬ TỰ ĐỘNG...';

  addCanLog('=== BẮT ĐẦU SMOKE TEST TỰ ĐỘNG (CAPL EQUIVALENT) ===', 'tx');
  
  // Step 1: Play
  sendCanCommand(1, 0, 0);
  addCanLog('[TEST STEP 1] Gửi lệnh PLAY (Cmd=0x01). Chờ 2.5 giây...', 'tx');

  setTimeout(() => {
    // Step 2: Fast Forward 10s
    sendCanCommand(0, 5, 10);
    addCanLog('[TEST STEP 2] Gửi lệnh FAST FORWARD +10s (Seek=0x05). Chờ 2.5 giây...', 'tx');
  }, 2500);

  setTimeout(() => {
    // Step 3: Next track
    sendCanCommand(3, 0, 0);
    addCanLog('[TEST STEP 3] Gửi lệnh NEXT TRACK (Cmd=0x03). Chờ 2.5 giây...', 'tx');
  }, 5000);

  setTimeout(() => {
    // Step 4: Pause
    sendCanCommand(2, 0, 0);
    addCanLog('[TEST STEP 4] Gửi lệnh PAUSE (Cmd=0x02).', 'tx');
    addCanLog('=== KẾT THÚC SMOKE TEST: HOÀN THÀNH TẤT CẢ 4 BƯỚC ===', 'rx');
    btn.disabled = false;
    btn.textContent = '🚀 CHẠY KỊCH BẢN KIỂM THỬ TỰ ĐỘNG (SMOKE TEST: Play → FF → Next → Pause)';
  }, 7500);
}

// --- OFFLINE PC MOCK SIMULATION MODE ---
function toggleMockMode() {
  enableMockMode(!mockMode);
}

function enableMockMode(enabled) {
  mockMode = enabled;
  if (mockMode) {
    btnMockToggle.textContent = 'Mock Mode: ON';
    btnMockToggle.style.borderColor = 'var(--color-cyan)';
    btnMockToggle.style.color = 'var(--color-cyan)';
    wsStatus.textContent = 'MOCK SIMULATION';
    wsStatus.className = 'badge badge-connected';
    startMockSimulation();
  } else {
    btnMockToggle.textContent = 'Mock Mode: OFF';
    btnMockToggle.style.borderColor = '';
    btnMockToggle.style.color = '';
    if (mockTimer) clearInterval(mockTimer);
    initWebSocket();
  }
}

function startMockSimulation() {
  if (mockTimer) clearInterval(mockTimer);
  mockTimer = setInterval(() => {
    mockAlive = (mockAlive + 1) & 0xFF;
    if (isPlaying) {
      mockCurTime += 1;
      if (mockCurTime > mockTotalTime) mockCurTime = 0;
    }
    handleTelemetryUpdate({
      type: 'telemetry',
      state: isPlaying ? 1 : 2,
      track: 1,
      curTime: mockCurTime,
      totalTime: mockTotalTime,
      usb: 2,
      alive: mockAlive
    });
  }, 1000);
}

function handleMockCommand(cmdId, seekCmd, seekVal) {
  const dummyCounter = (mockAlive % 16);
  const chksum = (cmdId + seekCmd + seekVal + dummyCounter) & 0xFF;
  addCanLog(`[MOCK TX 0x100] Cmd=${cmdId} Seek=${seekCmd} Val=${seekVal} Ctr=${dummyCounter} CRC=0x${chksum.toString(16).toUpperCase()}`, 'tx');

  if (cmdId === 1) isPlaying = true;
  if (cmdId === 2) isPlaying = false;
  if (seekCmd === 5) mockCurTime = Math.min(mockTotalTime, mockCurTime + seekVal);
  if (seekCmd === 6) mockCurTime = Math.max(0, mockCurTime - seekVal);
}

// Auto init on page load
window.addEventListener('DOMContentLoaded', () => {
  initWebSocket();
});
