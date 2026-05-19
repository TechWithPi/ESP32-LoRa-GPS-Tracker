#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

HardwareSerial LoRaSerial(1);

WebServer server(80);

// ===== YOUR WIFI =====
const char* ssid = "manish";
const char* password = "12345678";

String gpsData = "Waiting GPS...";

// ===== WEB PAGE =====
String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="https://unpkg.com/leaflet/dist/leaflet.css"/>
<script src="https://unpkg.com/leaflet/dist/leaflet.js"></script>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600;700;900&family=Share+Tech+Mono&display=swap" rel="stylesheet">

<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  :root {
    --cyan: #00ffe7;
    --red: #ff3c3c;
    --green: #39ff6e;
    --amber: #ffb800;
    --dim: rgba(0,255,231,0.08);
    --panel: rgba(0,10,20,0.85);
  }

  body {
    background: #000d1a;
    color: var(--cyan);
    font-family: 'Share Tech Mono', monospace;
    overflow: hidden;
    height: 100vh;
    width: 100vw;
  }

  /* ===== SCANLINE OVERLAY ===== */
  body::before {
    content: '';
    position: fixed;
    inset: 0;
    background: repeating-linear-gradient(
      0deg,
      transparent,
      transparent 2px,
      rgba(0,0,0,0.15) 2px,
      rgba(0,0,0,0.15) 4px
    );
    pointer-events: none;
    z-index: 9999;
  }

  /* ===== VIGNETTE ===== */
  body::after {
    content: '';
    position: fixed;
    inset: 0;
    background: radial-gradient(ellipse at center, transparent 50%, rgba(0,0,0,0.7) 100%);
    pointer-events: none;
    z-index: 9998;
  }

  /* ===== MAP ===== */
  #map {
    position: fixed;
    inset: 0;
    z-index: 0;
  }

  /* ===== TOP HEADER BAR ===== */
  #header {
    position: fixed;
    top: 0; left: 0; right: 0;
    height: 56px;
    background: var(--panel);
    border-bottom: 1px solid rgba(0,255,231,0.2);
    z-index: 1000;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0 20px;
    backdrop-filter: blur(8px);
  }

  .header-left {
    display: flex;
    align-items: center;
    gap: 14px;
  }

  .logo-text {
    font-family: 'Orbitron', monospace;
    font-size: 14px;
    font-weight: 900;
    letter-spacing: 4px;
    color: var(--cyan);
    text-shadow: 0 0 20px var(--cyan);
  }

  .header-divider {
    width: 1px;
    height: 28px;
    background: rgba(0,255,231,0.2);
  }

  .header-sub {
    font-size: 11px;
    color: rgba(0,255,231,0.5);
    letter-spacing: 2px;
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 20px;
  }

  /* ===== STATUS DOTS ===== */
  .status-item {
    display: flex;
    align-items: center;
    gap: 7px;
    font-size: 11px;
    color: rgba(0,255,231,0.6);
    letter-spacing: 1.5px;
  }

  .dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--green);
    box-shadow: 0 0 8px var(--green);
    animation: dotblink 1.4s ease-in-out infinite;
  }

  .dot.red { background: var(--red); box-shadow: 0 0 8px var(--red); animation: dotblink 0.8s ease-in-out infinite; }
  .dot.amber { background: var(--amber); box-shadow: 0 0 8px var(--amber); }

  @keyframes dotblink {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.2; }
  }

  /* ===== LEFT PANEL ===== */
  #left-panel {
    position: fixed;
    top: 56px;
    left: 0;
    width: 240px;
    bottom: 0;
    background: var(--panel);
    border-right: 1px solid rgba(0,255,231,0.12);
    z-index: 999;
    padding: 20px 16px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    backdrop-filter: blur(8px);
    overflow: hidden;
    transition: transform 0.35s cubic-bezier(0.4,0,0.2,1);
  }

  #left-panel.hidden {
    transform: translateX(-240px);
  }

  #panel-toggle {
    position: fixed;
    top: 50%;
    left: 240px;
    transform: translateY(-50%);
    z-index: 1100;
    background: var(--panel);
    border: 1px solid rgba(0,255,231,0.25);
    border-left: none;
    color: var(--cyan);
    font-family: 'Orbitron', monospace;
    font-size: 10px;
    font-weight: 700;
    writing-mode: vertical-rl;
    letter-spacing: 3px;
    padding: 14px 7px;
    cursor: pointer;
    transition: left 0.35s cubic-bezier(0.4,0,0.2,1), background 0.2s;
    backdrop-filter: blur(8px);
    text-shadow: 0 0 8px var(--cyan);
    user-select: none;
  }

  #panel-toggle:hover { background: rgba(0,255,231,0.1); }

  #panel-toggle.collapsed {
    left: 0;
    border-left: 1px solid rgba(0,255,231,0.25);
    border-right: none;
  }

  /* ===== PANEL SECTION ===== */
  .panel-section {
    border: 1px solid rgba(0,255,231,0.15);
    padding: 14px;
    position: relative;
  }

  /* corner brackets */
  .panel-section::before,
  .panel-section::after {
    content: '';
    position: absolute;
    width: 8px;
    height: 8px;
    border-color: var(--cyan);
    border-style: solid;
    opacity: 0.7;
  }
  .panel-section::before { top: -1px; left: -1px; border-width: 2px 0 0 2px; }
  .panel-section::after  { bottom: -1px; right: -1px; border-width: 0 2px 2px 0; }

  .section-label {
    font-size: 9px;
    letter-spacing: 3px;
    color: rgba(0,255,231,0.4);
    margin-bottom: 10px;
    text-transform: uppercase;
  }

  .coord-row {
    margin: 6px 0;
  }

  .coord-key {
    font-size: 10px;
    color: rgba(0,255,231,0.4);
    letter-spacing: 2px;
  }

  .coord-val {
    font-size: 16px;
    color: var(--cyan);
    text-shadow: 0 0 12px rgba(0,255,231,0.6);
    letter-spacing: 1px;
    font-family: 'Orbitron', monospace;
    font-weight: 600;
  }

  /* ===== SIGNAL BARS ===== */
  .signal-bars {
    display: flex;
    align-items: flex-end;
    gap: 4px;
    height: 28px;
    margin-top: 6px;
  }

  .signal-bars span {
    width: 10px;
    background: var(--cyan);
    box-shadow: 0 0 6px var(--cyan);
    border-radius: 1px;
    animation: signalpulse 2s ease-in-out infinite;
  }

  .signal-bars span:nth-child(1) { height: 30%; animation-delay: 0s; }
  .signal-bars span:nth-child(2) { height: 50%; animation-delay: 0.15s; }
  .signal-bars span:nth-child(3) { height: 70%; animation-delay: 0.3s; }
  .signal-bars span:nth-child(4) { height: 90%; animation-delay: 0.45s; }
  .signal-bars span:nth-child(5) { height: 100%; animation-delay: 0.6s; }

  @keyframes signalpulse {
    0%, 100% { opacity: 0.9; }
    50% { opacity: 0.3; }
  }

  /* ===== MINI RADAR ===== */
  #radar-canvas {
    width: 100%;
    aspect-ratio: 1;
    position: relative;
    margin-top: 4px;
  }

  #radar-canvas svg {
    width: 100%;
    height: 100%;
  }

  .radar-sweep {
    transform-origin: center;
    animation: sweep 3s linear infinite;
  }

  @keyframes sweep {
    from { transform: rotate(0deg); }
    to   { transform: rotate(360deg); }
  }

  /* ===== TRACKING STATUS ===== */
  .tracking-status {
    font-family: 'Orbitron', monospace;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 3px;
    color: var(--green);
    text-shadow: 0 0 10px var(--green);
    text-align: center;
    animation: statusflicker 2.5s ease-in-out infinite;
  }

  @keyframes statusflicker {
    0%, 95%, 100% { opacity: 1; }
    97% { opacity: 0.3; }
  }

  /* ===== BOTTOM BAR ===== */
  #bottom-bar {
    position: fixed;
    bottom: 0; left: 240px; right: 0;
    height: 48px;
    background: var(--panel);
    border-top: 1px solid rgba(0,255,231,0.12);
    z-index: 999;
    display: flex;
    align-items: center;
    padding: 0 20px;
    gap: 30px;
    backdrop-filter: blur(8px);
  }

  .stat-item {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }

  .stat-label {
    font-size: 8px;
    letter-spacing: 2px;
    color: rgba(0,255,231,0.35);
  }

  .stat-val {
    font-family: 'Orbitron', monospace;
    font-size: 13px;
    font-weight: 600;
    color: var(--cyan);
    text-shadow: 0 0 8px rgba(0,255,231,0.5);
  }

  .bar-divider {
    width: 1px;
    height: 28px;
    background: rgba(0,255,231,0.12);
  }

  /* ===== CORNER BRACKETS (full screen) ===== */
  .screen-corner {
    position: fixed;
    width: 24px;
    height: 24px;
    border-color: var(--cyan);
    border-style: solid;
    opacity: 0.35;
    z-index: 9997;
    pointer-events: none;
  }

  .sc-tl { top: 60px; left: 244px; border-width: 2px 0 0 2px; }
  .sc-tr { top: 60px; right: 4px;  border-width: 2px 2px 0 0; }
  .sc-bl { bottom: 52px; left: 244px; border-width: 0 0 2px 2px; }
  .sc-br { bottom: 52px; right: 4px;  border-width: 0 2px 2px 0; }

  /* ===== TIMESTAMP ===== */
  #timestamp {
    font-size: 10px;
    color: rgba(0,255,231,0.4);
    letter-spacing: 1.5px;
  }

  /* ===== LEAFLET CUSTOM MARKER ===== */
  .custom-marker {
    position: relative;
  }

  /* ===== PATH TRAIL GRADIENT override ===== */
  .leaflet-interactive {
    filter: drop-shadow(0 0 4px #ff3c3c);
  }

  /* ===== ZOOM CONTROLS ===== */
  .leaflet-control-zoom {
    border: 1px solid rgba(0,255,231,0.2) !important;
    border-radius: 0 !important;
    background: var(--panel) !important;
    backdrop-filter: blur(8px);
  }

  .leaflet-control-zoom a {
    background: transparent !important;
    color: var(--cyan) !important;
    border-bottom: 1px solid rgba(0,255,231,0.15) !important;
    font-family: 'Orbitron', monospace !important;
    font-size: 16px !important;
    width: 30px !important;
    height: 30px !important;
    line-height: 30px !important;
  }

  .leaflet-control-zoom a:hover {
    background: rgba(0,255,231,0.08) !important;
  }

  /* ===== SCROLL text animation ===== */
  .scroll-text {
    font-size: 9px;
    color: rgba(0,255,231,0.3);
    letter-spacing: 1px;
    white-space: nowrap;
    overflow: hidden;
    width: 100%;
    height: 16px;
    margin-top: auto;
  }

  .scroll-inner {
    display: inline-block;
    animation: scrolltxt 12s linear infinite;
  }

  @keyframes scrolltxt {
    from { transform: translateX(100%); }
    to   { transform: translateX(-100%); }
  }

</style>
</head>

<body>

<!-- Screen corner brackets -->
<div class="screen-corner sc-tl"></div>
<div class="screen-corner sc-tr"></div>
<div class="screen-corner sc-bl"></div>
<div class="screen-corner sc-br"></div>

<!-- ===== HEADER ===== -->
<div id="header">
  <div class="header-left">
    <div class="logo-text">LORA TRACKER</div>
    <div class="header-divider"></div>
    <div class="header-sub">REAL-TIME ASSET MONITOR v2.4</div>
  </div>
  <div class="header-right">
    <div class="status-item"><div class="dot"></div>SIGNAL ACTIVE</div>
    <div class="status-item"><div class="dot amber"></div>GPS LOCK</div>
    <div class="status-item"><div class="dot red"></div>TRANSMITTING</div>
    <div id="timestamp">--:--:--</div>
  </div>
</div>

<!-- ===== LEFT PANEL ===== -->
<div id="left-panel">

  <div class="tracking-status">◉ TRACKING ACTIVE</div>

  <div class="panel-section">
    <div class="section-label">▸ COORDINATES</div>
    <div class="coord-row">
      <div class="coord-key">LAT</div>
      <div class="coord-val" id="disp-lat">---.------</div>
    </div>
    <div class="coord-row">
      <div class="coord-key">LON</div>
      <div class="coord-val" id="disp-lon">---.------</div>
    </div>
  </div>

  <div class="panel-section">
    <div class="section-label">▸ SIGNAL STRENGTH</div>
    <div class="signal-bars">
      <span></span><span></span><span></span><span></span><span></span>
    </div>
    <div style="font-size:10px;color:rgba(0,255,231,0.4);margin-top:6px;letter-spacing:1px;">LORA 915MHz · RSSI GOOD</div>
  </div>

  <div class="panel-section">
    <div class="section-label">▸ RADAR</div>
    <div id="radar-canvas">
      <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
        <defs>
          <radialGradient id="rg" cx="50%" cy="50%">
            <stop offset="0%" stop-color="#00ffe7" stop-opacity="0.08"/>
            <stop offset="100%" stop-color="#00ffe7" stop-opacity="0"/>
          </radialGradient>
        </defs>
        <circle cx="50" cy="50" r="48" fill="none" stroke="rgba(0,255,231,0.1)" stroke-width="0.5"/>
        <circle cx="50" cy="50" r="36" fill="none" stroke="rgba(0,255,231,0.1)" stroke-width="0.5"/>
        <circle cx="50" cy="50" r="24" fill="none" stroke="rgba(0,255,231,0.1)" stroke-width="0.5"/>
        <circle cx="50" cy="50" r="12" fill="none" stroke="rgba(0,255,231,0.1)" stroke-width="0.5"/>
        <line x1="50" y1="2" x2="50" y2="98" stroke="rgba(0,255,231,0.08)" stroke-width="0.5"/>
        <line x1="2" y1="50" x2="98" y2="50" stroke="rgba(0,255,231,0.08)" stroke-width="0.5"/>
        <circle cx="50" cy="50" r="48" fill="url(#rg)"/>
        <g class="radar-sweep">
          <path d="M50 50 L50 2 A48 48 0 0 1 98 50 Z" fill="rgba(0,255,231,0.07)"/>
          <line x1="50" y1="50" x2="50" y2="2" stroke="rgba(0,255,231,0.6)" stroke-width="1"/>
        </g>
        <circle cx="50" cy="50" r="3" fill="#00ffe7" opacity="0.9"/>
        <circle cx="50" cy="50" r="3" fill="none" stroke="#00ffe7" stroke-width="1">
          <animate attributeName="r" values="3;14;3" dur="2s" repeatCount="indefinite"/>
          <animate attributeName="opacity" values="0.9;0;0.9" dur="2s" repeatCount="indefinite"/>
        </circle>
      </svg>
    </div>
  </div>

  <div class="panel-section" style="flex:1; display:flex; flex-direction:column; justify-content:space-between;">
    <div class="section-label">▸ LOG</div>
    <div id="log-area" style="font-size:9px;color:rgba(0,255,231,0.4);line-height:1.7;letter-spacing:0.5px;max-height:120px;overflow:hidden;"></div>
  </div>

  <div class="scroll-text">
    <span class="scroll-inner">SYS OK · LORA MODULE ACTIVE · GPS TRACKING · ENCRYPTED LINK · POSITION VERIFIED · </span>
  </div>

</div>

<!-- ===== PANEL TOGGLE ===== -->
<div id="panel-toggle" onclick="togglePanel()">&#9664; HUD</div>

<!-- ===== MAP ===== -->
<div id="map" style="left:240px; top:56px; bottom:48px; position:fixed; right:0; z-index:0; transition: left 0.35s cubic-bezier(0.4,0,0.2,1);"></div>

<!-- ===== BOTTOM BAR ===== -->
<div id="bottom-bar">
  <div class="stat-item">
    <div class="stat-label">POINTS LOGGED</div>
    <div class="stat-val" id="stat-points">0</div>
  </div>
  <div class="bar-divider"></div>
  <div class="stat-item">
    <div class="stat-label">LAST UPDATE</div>
    <div class="stat-val" id="stat-update">---</div>
  </div>
  <div class="bar-divider"></div>
  <div class="stat-item">
    <div class="stat-label">DISTANCE (m)</div>
    <div class="stat-val" id="stat-dist">0.0</div>
  </div>
  <div class="bar-divider"></div>
  <div class="stat-item">
    <div class="stat-label">STATUS</div>
    <div class="stat-val" style="color:var(--green);text-shadow:0 0 8px rgba(57,255,110,0.5);" id="stat-status">ONLINE</div>
  </div>
</div>

<script>

// ===== DEFAULT LOCATION =====
let lat = 28.6139;
let lon = 77.2090;
let pointCount = 0;
let totalDist = 0;
let prevLat = null, prevLon = null;

// ===== CREATE MAP =====
var map_obj = L.map('map', { zoomControl: true }).setView([lat, lon], 18);

// ===== MAP TILE =====
L.tileLayer(
  'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
  { maxZoom: 19 }
).addTo(map_obj);

// ===== CUSTOM MARKER SVG =====
var crosshairIcon = L.divIcon({
  html: `
    <div style="position:relative;width:40px;height:40px;">
      <svg viewBox="0 0 40 40" xmlns="http://www.w3.org/2000/svg" style="width:40px;height:40px;">
        <circle cx="20" cy="20" r="18" fill="none" stroke="rgba(0,255,231,0.25)" stroke-width="1"/>
        <circle cx="20" cy="20" r="18" fill="none" stroke="#00ffe7" stroke-width="0.5">
          <animate attributeName="r" values="8;20;8" dur="2s" repeatCount="indefinite"/>
          <animate attributeName="opacity" values="0.7;0;0.7" dur="2s" repeatCount="indefinite"/>
        </circle>
        <line x1="20" y1="2" x2="20" y2="10" stroke="#00ffe7" stroke-width="1.5"/>
        <line x1="20" y1="30" x2="20" y2="38" stroke="#00ffe7" stroke-width="1.5"/>
        <line x1="2" y1="20" x2="10" y2="20" stroke="#00ffe7" stroke-width="1.5"/>
        <line x1="30" y1="20" x2="38" y2="20" stroke="#00ffe7" stroke-width="1.5"/>
        <circle cx="20" cy="20" r="4" fill="#ff3c3c"/>
        <circle cx="20" cy="20" r="2" fill="#ffffff"/>
      </svg>
    </div>
  `,
  className: '',
  iconSize: [40, 40],
  iconAnchor: [20, 20]
});

var marker = L.marker([lat, lon], { icon: crosshairIcon }).addTo(map_obj);

// ===== PATH ARRAY =====
var pathCoordinates = [];

// ===== RED TRAIL LINE =====
var polyline = L.polyline(pathCoordinates, {
  color: '#ff3c3c',
  weight: 2,
  opacity: 0.8
}).addTo(map_obj);

// ===== HAVERSINE DISTANCE =====
function haversine(lat1, lon1, lat2, lon2) {
  var R = 6371000;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var a = Math.sin(dLat/2)*Math.sin(dLat/2) +
          Math.cos(lat1*Math.PI/180)*Math.cos(lat2*Math.PI/180)*
          Math.sin(dLon/2)*Math.sin(dLon/2);
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
}

// ===== LOG ===== 
var logLines = [];
function addLog(msg) {
  var t = new Date().toTimeString().split(' ')[0];
  logLines.unshift('[' + t + '] ' + msg);
  if (logLines.length > 8) logLines.pop();
  document.getElementById('log-area').innerHTML = logLines.join('<br>');
}

// ===== TIMESTAMP =====
setInterval(function() {
  document.getElementById('timestamp').textContent = new Date().toTimeString().split(' ')[0];
}, 1000);

// ===== UPDATE GPS =====
async function updateGPS() {
  try {
    let response = await fetch('/gps');
    let text = await response.text();

    let parts = text.split(",");
    if (parts.length >= 2) {
      let newLat = parseFloat(parts[0]);
      let newLon = parseFloat(parts[1]);

      if (!isNaN(newLat) && !isNaN(newLon)) {

        if (prevLat !== null) {
          var d = haversine(prevLat, prevLon, newLat, newLon);
          totalDist += d;
        }
        prevLat = newLat; prevLon = newLon;
        lat = newLat; lon = newLon;
        pointCount++;

        // Update displays
        document.getElementById('disp-lat').textContent = lat.toFixed(6);
        document.getElementById('disp-lon').textContent = lon.toFixed(6);
        document.getElementById('stat-points').textContent = pointCount;
        document.getElementById('stat-dist').textContent = totalDist.toFixed(1);
        document.getElementById('stat-update').textContent = new Date().toTimeString().split(' ')[0];

        // Move marker & map
        marker.setLatLng([lat, lon]);
        map_obj.setView([lat, lon]);

        // Add trail point
        pathCoordinates.push([lat, lon]);
        polyline.setLatLngs(pathCoordinates);

        addLog('POS UPDATE · ' + lat.toFixed(5) + ' , ' + lon.toFixed(5));
        document.getElementById('stat-status').textContent = 'ONLINE';
        document.getElementById('stat-status').style.color = 'var(--green)';
      }
    }
  } catch(e) {
    document.getElementById('stat-status').textContent = 'NO SIGNAL';
    document.getElementById('stat-status').style.color = 'var(--red)';
    addLog('SIGNAL LOST');
  }
}

// ===== PANEL TOGGLE =====
var panelOpen = true;
function togglePanel() {
  var panel = document.getElementById('left-panel');
  var btn   = document.getElementById('panel-toggle');
  var map   = document.getElementById('map');
  panelOpen = !panelOpen;
  if (panelOpen) {
    panel.classList.remove('hidden');
    btn.classList.remove('collapsed');
    btn.innerHTML = '&#9664; HUD';
    map.style.left = '240px';
  } else {
    panel.classList.add('hidden');
    btn.classList.add('collapsed');
    btn.innerHTML = '&#9654; HUD';
    map.style.left = '0px';
  }
  setTimeout(function(){ map_obj.invalidateSize(); }, 370);
}

addLog('SYSTEM BOOT');
addLog('LORA MODULE INIT');
addLog('AWAITING GPS DATA...');

// ===== UPDATE EVERY SECOND =====
setInterval(updateGPS, 1000);

</script>
</body>
</html>
)rawliteral";

// ===== HANDLE MAIN PAGE =====
void handleRoot() {
  server.send(200, "text/html", webpage);
}

// ===== HANDLE GPS =====
void handleGPS() {
  server.send(200, "text/plain", gpsData);
}

void setup() {
  Serial.begin(115200);

  // ===== LoRa RX TX =====
  LoRaSerial.begin(115200, SERIAL_8N1, 4, 5);

  // ===== CONNECT WIFI =====
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // ===== WEB ROUTES =====
  server.on("/", handleRoot);
  server.on("/gps", handleGPS);

  // ===== START SERVER =====
  server.begin();
  Serial.println("Web Server Started");
}

void loop() {
  server.handleClient();

  while (LoRaSerial.available()) {
    char c = LoRaSerial.read();
    static String receivedData = "";
    receivedData += c;

    // ===== FULL DATA RECEIVED =====
    if (c == '\n') {
      Serial.println(receivedData);

      // Example:
      // +RCV=1,20,28.613939,77.209021,-45,40
      int firstComma  = receivedData.indexOf(',');
      int secondComma = receivedData.indexOf(',', firstComma + 1);
      int thirdComma  = receivedData.indexOf(',', secondComma + 1);
      int fourthComma = receivedData.indexOf(',', thirdComma + 1);

      // ===== EXTRACT GPS =====
      gpsData = receivedData.substring(secondComma + 1, fourthComma);

      Serial.println("GPS:");
      Serial.println(gpsData);

      // ===== CLEAR BUFFER =====
      receivedData = "";
    }
  }
}
