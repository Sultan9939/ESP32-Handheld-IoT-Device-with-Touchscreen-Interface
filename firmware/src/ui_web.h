#pragma once
#include <WebServer.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 Handheld</title>
  <style>
    body{font-family:system-ui;margin:20px}
    .row{margin:10px 0}
    button{padding:12px 16px;margin-right:8px}
    .card{padding:12px;border:1px solid #ddd;border-radius:12px;max-width:420px}
    .big{font-size:20px;font-weight:600}
    .mono{font-family:ui-monospace, SFMono-Regular, Menlo, monospace}
  </style>
</head>
<body>
  <div class="card">
    <div class="row big">ESP32 IoT Handheld (Home)</div>
    <div class="row">State: <span id="state" class="mono">...</span></div>
    <div class="row">Temp: <span id="t" class="mono">...</span> &deg;C</div>
    <div class="row">Hum: <span id="h" class="mono">...</span> %</div>
    <div class="row">RPM: <span id="rpm" class="mono">...</span></div>

    <div class="row">
      <button onclick="cmd('start')">Start</button>
      <button onclick="cmd('stop')">Stop</button>
      <button onclick="cmd('reset')">Reset</button>
    </div>
  </div>

<script>
async function refresh(){
  const r = await fetch('/api/sensor');
  const d = await r.json();
  document.getElementById('state').textContent = d.state;
  document.getElementById('t').textContent = d.temperature_c.toFixed(1);
  document.getElementById('h').textContent = d.humidity_pct.toFixed(1);
  document.getElementById('rpm').textContent = d.rpm;
}
async function cmd(name){
  await fetch('/api/'+name, {method:'POST'});
  await refresh();
}
setInterval(refresh, 1500);
refresh();
</script>
</body>
</html>
)rawliteral";

inline void registerUI(WebServer& server) {
  server.on("/", HTTP_GET, [&server]() {
    server.send(200, "text/html; charset=utf-8", INDEX_HTML);
  });
}
