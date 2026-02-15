#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "iPhone (NURSULTAN)";
const char* password = "qwertyui";

WebServer server(80);

enum class DeviceState { IDLE, RUNNING, STOPPED };
DeviceState currentState = DeviceState::IDLE;

// --- Forward declarations (важно для Arduino) ---
String stateToString();

void handleRoot();
void handleState();
void handleStart();
void handleStop();
void handleReset();
void handleSensor();

float simTemperatureC();
float simHumidityPct();
int   simRpm();

// --- "Device" cached values (freeze when stopped) ---
float lastTemp = 0.0;
float lastHum  = 0.0;
int   lastRpm  = 0;
unsigned long lastUpdate = 0;

// --- Web UI (Home) ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
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

// --- Helpers / Handlers ---
String stateToString() {
  switch (currentState) {
    case DeviceState::IDLE: return "IDLE";
    case DeviceState::RUNNING: return "RUNNING";
    case DeviceState::STOPPED: return "STOPPED";
  }
  return "UNKNOWN";
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleState() {
  String json = "{ \"state\": \"" + stateToString() + "\" }";
  server.send(200, "application/json", json);
}

void handleStart() {
  currentState = DeviceState::RUNNING;
  server.send(200, "application/json", "{ \"ok\": true, \"state\": \"RUNNING\" }");
}

void handleStop() {
  currentState = DeviceState::STOPPED;
  server.send(200, "application/json", "{ \"ok\": true, \"state\": \"STOPPED\" }");
}

void handleReset() {
  currentState = DeviceState::IDLE;

  // optional: reset cached values
  lastTemp = 0.0;
  lastHum  = 0.0;
  lastRpm  = 0;

  server.send(200, "application/json", "{ \"ok\": true, \"state\": \"IDLE\" }");
}

float simTemperatureC() { return random(200, 350) / 10.0; } // 20.0–35.0
float simHumidityPct()  { return random(300, 800) / 10.0; } // 30.0–80.0
int   simRpm()          { return random(800, 2200); }

void handleSensor() {
  String json = "{";
  json += "\"temperature_c\":" + String(lastTemp, 1) + ",";
  json += "\"humidity_pct\":"  + String(lastHum, 1)  + ",";
  json += "\"rpm\":"           + String(lastRpm)     + ",";
  json += "\"ts_ms\":"         + String(millis())    + ",";
  json += "\"state\":\""       + stateToString()     + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// --- Setup / Loop ---
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  randomSeed(esp_random());

  // Register routes AFTER WiFi is up
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/start", HTTP_POST, handleStart);
  server.on("/api/stop",  HTTP_POST, handleStop);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/sensor", HTTP_GET, handleSensor);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Update simulated data only in RUNNING (freeze otherwise)
  if (currentState == DeviceState::RUNNING) {
    if (millis() - lastUpdate > 500) {
      lastTemp = simTemperatureC();
      lastHum  = simHumidityPct();
      lastRpm  = simRpm();
      lastUpdate = millis();
    }
  }
}
