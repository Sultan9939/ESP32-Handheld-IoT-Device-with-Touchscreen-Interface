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
    input{padding:10px 12px; width: 100%; box-sizing: border-box;}
    .card{padding:12px;border:1px solid #ddd;border-radius:12px;max-width:520px}
    .big{font-size:20px;font-weight:600}
    .mono{font-family:ui-monospace, SFMono-Regular, Menlo, monospace}
    .grid{display:grid; gap:10px}
    .two{grid-template-columns: 1fr 1fr}
    .muted{opacity:.75}
    .pill{display:inline-block;padding:3px 8px;border:1px solid #ddd;border-radius:999px;font-size:12px}
    .hr{height:1px;background:#eee;margin:14px 0}
  </style>
</head>
<body>
  <div class="card">
    <div class="row big">ESP32 IoT Handheld <span class="pill" id="devname">...</span></div>

    <div class="row">State: <span id="state" class="mono">...</span></div>

    <div class="grid two">
      <div class="row">Temp: <span id="t" class="mono">...</span> &deg;C</div>
      <div class="row">Hum: <span id="h" class="mono">...</span> %</div>
    </div>
    <div class="row">RPM: <span id="rpm" class="mono">...</span></div>

    <div class="row">
      <button onclick="cmd('start')">Start</button>
      <button onclick="cmd('stop')">Stop</button>
      <button onclick="cmd('reset')">Reset</button>
    </div>

    <div class="hr"></div>

    <div class="row big">Settings <span class="muted" id="saveStatus"></span></div>

    <div class="grid">
      <div>
        <div class="muted">Device name</div>
        <input id="s_name" placeholder="ESP32-Handheld" maxlength="32" />
      </div>

      <div class="grid two">
        <div>
          <div class="muted">Update interval (ms)</div>
          <input id="s_interval" type="number" min="200" max="2000" step="50" />
        </div>
        <div>
          <div class="muted">RPM limit</div>
          <input id="s_rpmlimit" type="number" min="800" max="5000" step="50" />
        </div>
      </div>

      <div class="row">
        <button onclick="saveSettings()">Save settings</button>
        <button onclick="loadSettings()">Reload</button>
      </div>
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
  document.getElementById('devname').textContent = d.device_name || '...';
}

async function cmd(name){
  await fetch('/api/'+name, {method:'POST'});
  await refresh();
}

async function loadSettings(){
  const r = await fetch('/api/settings');
  const s = await r.json();
  document.getElementById('s_name').value = s.device_name ?? '';
  document.getElementById('s_interval').value = s.update_interval_ms ?? 500;
  document.getElementById('s_rpmlimit').value = s.rpm_limit ?? 2200;
  document.getElementById('saveStatus').textContent = '';
}

async function saveSettings(){
  const payload = {
    device_name: document.getElementById('s_name').value,
    update_interval_ms: Number(document.getElementById('s_interval').value),
    rpm_limit: Number(document.getElementById('s_rpmlimit').value)
  };
  document.getElementById('saveStatus').textContent = 'saving...';

  const r = await fetch('/api/settings', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  });

  const res = await r.json();
  document.getElementById('saveStatus').textContent = res.ok ? 'saved ✅' : 'error';
  await refresh();
}

setInterval(refresh, 1500);
loadSettings();
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
