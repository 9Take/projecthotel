#ifndef SECRETS_H
#define SECRETS_H

// 1. ตั้งค่า Wi-Fi
const char* SECRET_SSID = "OFA";
const char* SECRET_WIFI_PASS = "gowzalnw555";

// 2. ตั้งค่า MQTT Broker
const char* SECRET_MQTT_SERVER = "recasa888.duckdns.org";
const int   SECRET_MQTT_PORT = 1883;
const char* SECRET_MQTT_USER = "m5stack";
const char* SECRET_MQTT_PASS = "1234";

// 3. ตั้งค่า Security & รหัส Admin
const String SECRET_HASH_KEY = "LUMINA2026"; 
const String SECRET_ADMIN_PIN = "1234";

#endif