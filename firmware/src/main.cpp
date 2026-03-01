#include <WiFi.h>
#include <WebServer.h>
#include "ui_web.h"

const char* ssid = "iPhone (NURSULTAN)";
const char* password = "qwertyui";

WebServer server(80);

enum class DeviceState { IDLE, RUNNING, STOPPED };
DeviceState currentState = DeviceState::IDLE;

// ---- Settings (RAM) ----
struct Settings {
  uint32_t update_interval_ms = 500;  // 200..2000 recommended
  int rpm_limit = 2200;              // 800..5000 reasonable
  String device_name = "ESP32-Handheld";
} settings;

// ---- Cached sensor values ----
float lastTemp = 0.0;
float lastHum  = 0.0;
int   lastRpm  = 0;
unsigned long lastUpdate = 0;

String stateToString() {
  switch (currentState) {
    case DeviceState::IDLE: return "IDLE";
    case DeviceState::RUNNING: return "RUNNING";
    case DeviceState::STOPPED: return "STOPPED";
  }
  return "UNKNOWN";
}

// ---- Simple simulated sensors ----
float simTemperatureC() { return random(200, 350) / 10.0; } // 20.0–35.0
float simHumidityPct()  { return random(300, 800) / 10.0; } // 30.0–80.0
int   simRpmRaw()       { return random(800, 3000); }

// ---- Tiny helpers for parsing JSON (no ArduinoJson needed) ----
static bool extractInt(const String& body, const char* key, int& out) {
  String k = String("\"") + key + "\":";
  int i = body.indexOf(k);
  if (i < 0) return false;
  i += k.length();
  while (i < (int)body.length() && (body[i] == ' ')) i++;
  int j = i;
  if (j < (int)body.length() && (body[j] == '-')) j++;
  while (j < (int)body.length() && isDigit(body[j])) j++;
  if (j == i) return false;
  out = body.substring(i, j).toInt();
  return true;
}

static bool extractString(const String& body, const char* key, String& out) {
  String k = String("\"") + key + "\":";
  int i = body.indexOf(k);
  if (i < 0) return false;
  i += k.length();
  while (i < (int)body.length() && body[i] == ' ') i++;
  if (i >= (int)body.length() || body[i] != '"') return false;
  i++;
  int j = body.indexOf('"', i);
  if (j < 0) return false;
  out = body.substring(i, j);
  return true;
}

static void clampSettings() {
  if (settings.update_interval_ms < 200) settings.update_interval_ms = 200;
  if (settings.update_interval_ms > 2000) settings.update_interval_ms = 2000;

  if (settings.rpm_limit < 800) settings.rpm_limit = 800;
  if (settings.rpm_limit > 5000) settings.rpm_limit = 5000;

  if (settings.device_name.length() == 0) settings.device_name = "ESP32-Handheld";
  if (settings.device_name.length() > 32) settings.device_name = settings.device_name.substring(0, 32);
}

// ---- API handlers ----
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
  lastTemp = lastHum = 0;
  lastRpm = 0;
  server.send(200, "application/json", "{ \"ok\": true, \"state\": \"IDLE\" }");
}

void handleSensor() {
  String json = "{";
  json += "\"temperature_c\":" + String(lastTemp, 1) + ",";
  json += "\"humidity_pct\":"  + String(lastHum, 1)  + ",";
  json += "\"rpm\":"           + String(lastRpm)     + ",";
  json += "\"ts_ms\":"         + String(millis())    + ",";
  json += "\"state\":\""       + stateToString()     + "\",";
  json += "\"device_name\":\"" + settings.device_name + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleGetSettings() {
  String json = "{";
  json += "\"device_name\":\"" + settings.device_name + "\",";
  json += "\"update_interval_ms\":" + String(settings.update_interval_ms) + ",";
  json += "\"rpm_limit\":" + String(settings.rpm_limit);
  json += "}";
  server.send(200, "application/json", json);
}

void handlePostSettings() {
  String body = server.arg("plain"); // raw request body
  bool changed = false;

  int v = 0;
  String s;

  if (extractInt(body, "update_interval_ms", v)) {
    settings.update_interval_ms = (uint32_t)v;
    changed = true;
  }
  if (extractInt(body, "rpm_limit", v)) {
    settings.rpm_limit = v;
    changed = true;
  }
  if (extractString(body, "device_name", s)) {
    settings.device_name = s;
    changed = true;
  }

  clampSettings();

  String json = "{ \"ok\": true, \"changed\": ";
  json += (changed ? "true" : "false");
  json += ", \"settings\": ";
  json += "{";
  json += "\"device_name\":\"" + settings.device_name + "\",";
  json += "\"update_interval_ms\":" + String(settings.update_interval_ms) + ",";
  json += "\"rpm_limit\":" + String(settings.rpm_limit);
  json += "}}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  randomSeed(esp_random());
  clampSettings();

  // API
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/start", HTTP_POST, handleStart);
  server.on("/api/stop",  HTTP_POST, handleStop);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/sensor", HTTP_GET, handleSensor);

  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handlePostSettings);

  // UI
  registerUI(server);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Update simulated data only in RUNNING (freeze otherwise)
  if (currentState == DeviceState::RUNNING) {
    if (millis() - lastUpdate > settings.update_interval_ms) {
      lastTemp = simTemperatureC();
      lastHum  = simHumidityPct();

      int raw = simRpmRaw();
      lastRpm = (raw > settings.rpm_limit) ? settings.rpm_limit : raw;

      lastUpdate = millis();
    }
  }
}
