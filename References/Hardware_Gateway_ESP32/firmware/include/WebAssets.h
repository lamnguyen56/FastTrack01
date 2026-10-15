#pragma once

#include <Arduino.h>

// ============================================================================
// DỰ PHÒNG TỰ ĐỘNG: GIAO DIỆN WEB ĐƯỢC LƯU TRONG PROGMEM FLASH
// (Dùng khi người dùng chưa nạp thư mục LittleFS qua lệnh uploadfs)
// ============================================================================

const char FALLBACK_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 CAN Gateway | Automotive Cockpit HMI</title>
<style>
  :root { --bg-dark:#0a0c10; --bg-card:#161b22; --border:#30363d; --cyan:#00e5ff; --green:#2ecc71; --amber:#f39c12; --red:#e74c3c; --text:#f0f6fc; --muted:#8b949e; }
  * { box-sizing:border-box; margin:0; padding:0; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif; }
  body { background:var(--bg-dark); color:var(--text); padding:16px; display:flex; justify-content:center; }
  .app-container { width:100%; max-width:920px; display:flex; flex-direction:column; gap:16px; }
  .header { display:flex; justify-content:space-between; align-items:center; background:var(--bg-card); border:1px solid var(--border); padding:14px 18px; border-radius:12px; }
  .header h1 { font-size:1.15rem; color:var(--cyan); letter-spacing:1px; }
  .badge { font-size:0.75rem; font-weight:700; padding:4px 10px; border-radius:16px; text-transform:uppercase; border:1px solid transparent; }
  .badge-on { color:var(--green); border-color:var(--green); background:rgba(46,204,113,0.15); }
  .badge-off { color:var(--red); border-color:var(--red); background:rgba(231,76,60,0.15); }
  .grid { display:grid; grid-template-columns:1fr 1fr; gap:16px; }
  @media(max-width:768px){ .grid { grid-template-columns:1fr; } }
  .card { background:var(--bg-card); border:1px solid var(--border); border-radius:12px; padding:18px; box-shadow:0 6px 20px rgba(0,0,0,0.3); }
  .card h2 { font-size:0.9rem; color:var(--muted); text-transform:uppercase; margin-bottom:12px; border-bottom:1px solid rgba(48,54,61,0.5); padding-bottom:6px; }
  .row { display:flex; justify-content:space-between; margin-bottom:10px; font-size:0.9rem; }
  .val { font-weight:700; font-family:monospace; }
  .time-box { display:flex; justify-content:space-between; font-family:monospace; font-size:0.9rem; color:var(--cyan); margin:12px 0 6px; }
  .prog-bg { width:100%; height:8px; background:#21262d; border-radius:4px; overflow:hidden; }
  .prog-fill { height:100%; width:0%; background:linear-gradient(90deg, #0077b6, var(--cyan)); transition:width 0.3s ease; }
  .console { background:#05070a; border:1px solid #1c2128; border-radius:8px; height:180px; overflow-y:auto; padding:10px; font-family:monospace; font-size:0.78rem; display:flex; flex-direction:column; gap:4px; }
  .log-tx { color:var(--cyan); } .log-rx { color:var(--green); } .log-info { color:var(--muted); }
  .btn-grid { display:grid; grid-template-columns:repeat(5,1fr); gap:10px; margin-bottom:14px; }
  @media(max-width:600px){ .btn-grid { grid-template-columns:repeat(3,1fr); } }
  .btn { background:#21262d; border:1px solid var(--border); color:var(--text); padding:12px 6px; border-radius:8px; cursor:pointer; font-weight:700; font-size:0.85rem; display:flex; flex-direction:column; align-items:center; gap:4px; }
  .btn:hover { border-color:var(--cyan); color:var(--cyan); }
  .btn-play { background:#0077b6; border-color:#0096c7; color:#fff; }
  .btn-play:hover { background:#0096c7; color:#fff; }
  .btn-auto { width:100%; background:linear-gradient(90deg, #6a0dad, #9b59b6); border:1px solid #af7ac5; color:#fff; padding:12px; font-weight:700; border-radius:8px; cursor:pointer; font-size:0.85rem; }
  .btn-auto:hover { filter:brightness(1.15); }
</style>
</head>
<body>
<div class="app-container">
  <div class="header">
    <h1>🏎️ FASTTRACK-01 CAN GATEWAY</h1>
    <span id="wsStatus" class="badge badge-off">WS: DISCONNECTED</span>
  </div>

  <div class="grid">
    <div class="card">
      <h2>📊 IVI Cluster Telemetry (CAN ID 0x101)</h2>
      <div class="row"><span>Trạng thái phát:</span><span id="stPlay" class="val" style="color:var(--muted)">STOPPED</span></div>
      <div class="row"><span>Thiết bị USB:</span><span id="stUsb" class="val" style="color:var(--red)">DISCONNECTED</span></div>
      <div class="row"><span>Track Index:</span><span id="stTrack" class="val">#00</span></div>
      <div class="row"><span>Heartbeat Pulse:</span><span id="stAlive" class="val">0</span></div>
      <div class="time-box"><span id="tCur">00:00</span><span id="tTot">00:00</span></div>
      <div class="prog-bg"><div id="progFill" class="prog-fill"></div></div>
    </div>

    <div class="card">
      <h2>📡 Live CAN Bus Traffic Sniffer</h2>
      <div class="console" id="consoleBox"><div class="log-info">[INIT] Đang kết nối mạng WebSocket...</div></div>
    </div>
  </div>

  <div class="card">
    <h2>🎮 Steering Wheel Media Controls (CAN ID 0x100)</h2>
    <div class="btn-grid">
      <button class="btn" onclick="sendCmd(4,0,0)"><span>⏮</span><span>PREV</span></button>
      <button class="btn" onclick="sendCmd(0,6,10)"><span>◀◀</span><span>-10s</span></button>
      <button class="btn btn-play" id="btnPlay" onclick="togglePlay()"><span>▶</span><span id="txtPlay">PLAY</span></button>
      <button class="btn" onclick="sendCmd(0,5,10)"><span>▶▶</span><span>+10s</span></button>
      <button class="btn" onclick="sendCmd(3,0,0)"><span>⏭</span><span>NEXT</span></button>
    </div>
    <button class="btn-auto" id="btnTest" onclick="runAutoTest()">🚀 CHẠY KỊCH BẢN KIỂM THỬ TỰ ĐỘNG (SMOKE TEST)</button>
  </div>
</div>

<script>
  let ws, isPlaying = false;
  const wsStatus = document.getElementById('wsStatus');
  const stPlay = document.getElementById('stPlay'), stUsb = document.getElementById('stUsb');
  const stTrack = document.getElementById('stTrack'), stAlive = document.getElementById('stAlive');
  const tCur = document.getElementById('tCur'), tTot = document.getElementById('tTot');
  const progFill = document.getElementById('progFill'), txtPlay = document.getElementById('txtPlay');
  const consoleBox = document.getElementById('consoleBox');

  function fmtTime(s){ let m=Math.floor(s/60),sec=s%60; return (m<10?'0':'')+m+':'+(sec<10?'0':'')+sec; }
  function log(txt,cls){ let d=document.createElement('div'); d.className='log-'+cls; d.textContent='['+new Date().toTimeString().split(' ')[0]+'] '+txt; consoleBox.appendChild(d); consoleBox.scrollTop=consoleBox.scrollHeight; }

  function initWS(){
    ws = new WebSocket('ws://' + window.location.hostname + '/ws');
    ws.onopen = () => { wsStatus.textContent = 'WS: CONNECTED'; wsStatus.className = 'badge badge-on'; log('WebSocket kết nối thành công tới ESP32!', 'rx'); };
    ws.onclose = () => { wsStatus.textContent = 'WS: DISCONNECTED'; wsStatus.className = 'badge badge-off'; setTimeout(initWS, 2500); };
    ws.onmessage = (e) => {
      let d = JSON.parse(e.data);
      if(d.type === 'telemetry'){
        stAlive.textContent = d.alive; stTrack.textContent = '#' + (d.track<10?'0':'') + d.track;
        tCur.textContent = fmtTime(d.curTime); tTot.textContent = fmtTime(d.totalTime);
        let pct = d.totalTime > 0 ? (d.curTime / d.totalTime)*100 : 0;
        progFill.style.width = pct + '%';
        isPlaying = (d.state === 1);
        stPlay.textContent = d.state === 1 ? 'PLAYING' : (d.state === 2 ? 'PAUSED' : 'STOPPED');
        stPlay.style.color = d.state === 1 ? 'var(--green)' : (d.state === 2 ? 'var(--amber)' : 'var(--muted)');
        txtPlay.textContent = isPlaying ? 'PAUSE' : 'PLAY';
        stUsb.textContent = d.usb === 2 ? 'READY' : (d.usb === 1 ? 'SCANNING' : 'DISCONNECTED');
        stUsb.style.color = d.usb === 2 ? 'var(--green)' : 'var(--red)';
      } else if(d.type === 'log'){ log(d.msg, d.isTx ? 'tx' : 'rx'); }
    };
  }

  function sendCmd(cmd,seek,val){ if(ws && ws.readyState===1) ws.send(JSON.stringify({action:'cmd',cmd,seek,val})); }
  function togglePlay(){ sendCmd(isPlaying ? 2 : 1, 0, 0); }
  function runAutoTest(){
    log('=== BẮT ĐẦU SMOKE TEST TỰ ĐỘNG ===', 'tx');
    sendCmd(1,0,0);
    setTimeout(()=>sendCmd(0,5,10), 2500);
    setTimeout(()=>sendCmd(3,0,0), 5000);
    setTimeout(()=>{ sendCmd(2,0,0); log('=== SMOKE TEST HOÀN TẤT ===', 'rx'); }, 7500);
  }
  window.onload = initWS;
</script>
</body>
</html>
)rawliteral";
