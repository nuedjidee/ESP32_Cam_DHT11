/*
  ESP32-CAM + DHT11: วัดอุณหภูมิ/ความชื้น และดูบนมือถือแบบ Standalone
  บอร์ด: AI Thinker ESP32-CAM | ไลบรารี: DHT sensor library by Adafruit
  สาย: DHT11 VCC->3V3, DATA->GPIO13, GND->GND
  ตัวเปล่า 4 ขา: เพิ่ม R 10k ระหว่าง DATA กับ 3V3; ขา NC ไม่ต่อ
  โปรเจคนี้ไม่เริ่มกล้อง ไม่ใช้ MicroSD และไม่ต้องต่ออินเทอร์เน็ต
*/
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <math.h>
#include "config.h"
#include "web_page.h"

DHT dht(DHT_PIN, DHT11);
WebServer server(80);
float temperatureC = NAN;
float humidityRH = NAN;
bool attempted = false;
bool sampleValid = false;
bool apReady = false;
uint32_t lastAttemptMs = 0;
uint32_t sequence = 0;

// อ่านเป็นรอบด้วย millis(): หน้าเว็บจึงยังตอบคำขอได้ระหว่างรออ่านครั้งถัดไป
void updateSensor() {
  const uint32_t now = millis();
  if (uint32_t(now - lastAttemptMs) < SENSOR_INTERVAL_MS) return;
  lastAttemptMs = now;
  attempted = true;
  ++sequence;
  const float h = dht.readHumidity();
  const float t = dht.readTemperature(); // องศาเซลเซียส
  sampleValid = isfinite(t) && isfinite(h) && h >= 0 && h <= 100;
  // เมื่ออ่านไม่ได้ ล้างค่าเดิม เพื่อไม่ให้เข้าใจว่าเป็นค่าปัจจุบัน
  temperatureC = sampleValid ? t : NAN;
  humidityRH = sampleValid ? h : NAN;
  if (sampleValid) {
    Serial.printf("DHT11: %.1f C | %.1f %%RH\n", t, h);
  } else {
    Serial.println("DHT11 read failed: check DATA GPIO13, VCC 3V3, GND and pull-up.");
  }
}

void sendSensorJson() {
  const uint32_t age = uint32_t(millis() - lastAttemptMs);
  const bool fresh = attempted && sampleValid && age <= SENSOR_INTERVAL_MS * 3UL;
  const char *state = !attempted ? "waiting" : (!sampleValid ? "sensor_error" : (fresh ? "ok" : "stale"));
  String json;
  json.reserve(200);
  json = "{\"status\":\"";
  json += state;
  json += "\",\"temperature\":";
  json += fresh ? String(temperatureC, 1) : String("null");
  json += ",\"humidity\":";
  json += fresh ? String(humidityRH, 1) : String("null");
  json += ",\"sequence\":";
  json += String(sequence);
  json += ",\"age_ms\":";
  json += attempted ? String(age) : String("null");
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", json);
}

void setup() {
  Serial.begin(115200);
  // ปิดไฟแฟลชของ ESP32-CAM เพื่อลดความร้อนใกล้เซนเซอร์
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  dht.begin();
  lastAttemptMs = millis(); // รอให้เซนเซอร์พร้อมก่อนอ่านครั้งแรก

  WiFi.mode(WIFI_AP);
  const IPAddress ip(192, 168, 4, 1);
  const IPAddress mask(255, 255, 255, 0);
  if (!WiFi.softAPConfig(ip, ip, mask)) {
    Serial.println("AP IP configuration failed. Restart the board.");
    return;
  }
  apReady = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (!apReady) {
    Serial.println("AP failed: check SSID and password (8-63 characters).");
    return;
  }
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/api", HTTP_GET, sendSensorJson);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.printf("\nWi-Fi: %s\nOpen http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void loop() {
  if (apReady) server.handleClient();
  updateSensor();
  delay(2); // ยอมให้ระบบเครือข่ายทำงาน ไม่รออ่านเซนเซอร์ด้วย delay ยาว
}
