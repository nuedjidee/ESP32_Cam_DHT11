#pragma once

// ชื่อและรหัส Wi-Fi ที่บอร์ดสร้างให้มือถือเชื่อมต่อ (รหัสอย่างน้อย 8 ตัว)
constexpr char AP_SSID[] = "ESP32-DHT11";
constexpr char AP_PASSWORD[] = "12345678";

// AI Thinker ESP32-CAM: GPIO13 ใช้ร่วมกับ MicroSD จึงห้ามใช้ MicroSD ในโปรเจคนี้
constexpr uint8_t DHT_PIN = 13;
constexpr uint32_t SENSOR_INTERVAL_MS = 2500;
static_assert(SENSOR_INTERVAL_MS >= 2000, "Read DHT no faster than every 2 seconds");

static_assert(sizeof(AP_PASSWORD) >= 9 && sizeof(AP_PASSWORD) <= 64, "Wi-Fi password: 8-63 ASCII characters");
