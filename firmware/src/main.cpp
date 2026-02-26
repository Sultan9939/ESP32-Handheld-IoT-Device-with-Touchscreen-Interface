#include <WiFi.h>
#include <WebServer.h>
#include "ui_web.h"

const char* ssid = "iPhone (NURSULTAN)";
const char* password = "qwertyui";

WebServer server(80);

enum class DeviceState { IDLE, RUNNING, STOPPED };
DeviceState currentState = DeviceState::IDLE;

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

float simTemperatureC() { return random(200, 350) / 10.0; }
float simHumidityPct()  { return random(300, 800) / 10.0; }
int   simRpm()          { return random(800, 2200); }

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
  json += "\"state\":\""       + stateToString()     + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  randomSeed(esp_random());

  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/start", HTTP_POST, handleStart);
  server.on("/api/stop",  HTTP_POST, handleStop);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/sensor", HTTP_GET, handleSensor);

  registerUI(server);

  server.begin();
}

void loop() {
  server.handleClient();

  if (currentState == DeviceState::RUNNING && millis() - lastUpdate > 500) {
    lastTemp = simTemperatureC();
    lastHum  = simHumidityPct();
    lastRpm  = simRpm();
    lastUpdate = millis();
  }
}
