/*
 * ============================================================================
 * Project: ForgeSense - ESP8266 Wi-Fi Telemetry Bridge
 * Target: ESP8266 (ESP-01 / ESP-12E / NodeMCU)
 * Function: Reads 9-field CSV telemetry from STM32 USART2 (9600 Baud)
 *           and serves a live HTTP Web Dashboard over Wi-Fi.
 * ============================================================================
 * WIRING:
 * - STM32 PA2 (USART2_TX) -> ESP8266 RX0 (GPIO3)
 * - STM32 PA3 (USART2_RX) -> ESP8266 TX0 (GPIO1)
 * - GND -> Common Ground
 * - VCC -> 3.3V Dedicated Rail (min 300 mA)
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid     = "ForgeSense_AP";
const char* password = "password123";

ESP8266WebServer server(80);

String latestTelemetry = "0,0,0,0,0,0,STOPPED,0,0";

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='1'>";
  html += "<title>ForgeSense Wi-Fi Dashboard</title>";
  html += "<style>body{font-family:sans-serif;background:#0f172a;color:#fff;padding:20px;}";
  html += ".card{background:#1e293b;padding:15px;margin:10px 0;border-radius:8px;}";
  html += "h1{color:#38bdf8;}</style></head><body>";
  html += "<h1>ForgeSense Wi-Fi Telemetry</h1>";
  html += "<div class='card'><h3>Raw Telemetry Data</h3><p>" + latestTelemetry + "</p></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  server.send(200, "text/plain", latestTelemetry);
}

void setup() {
  Serial.begin(9600);
  
  WiFi.softAP(ssid, password);
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      latestTelemetry = line;
    }
  }
}
