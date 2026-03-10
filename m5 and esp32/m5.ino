#include <M5CoreS3.h>
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <time.h> 
#include <Preferences.h> 

// 🌟 นำเข้าไลบรารีเข้ารหัส Hash
#include "mbedtls/md.h"

const char* SECRET_SSID;
const char* SECRET_WIFI_PASS;
const char* SECRET_MQTT_SERVER;
const int SECRET_MQTT_PORT;
const char* SECRET_MQTT_USER;
const char* SECRET_MQTT_PASS;

// 🌟 รหัสลับสำหรับทำ Hash (ต้องตรงกับเว็บของเพื่อน)
const String SECRET_KEY = "LUMINA2026"; 

WiFiClient espClient;
PubSubClient mqttClient(espClient);
Preferences prefs; 

uint8_t nodeAddress[] = {0x14, 0x2B, 0x2F, 0xC0, 0xAF, 0xDC}; 

typedef struct SensorData {
  int node_id;
  bool rfid_detected;
  char rfid_uid[20];
  int door_status;
  float current_value;    
  bool is_sensor_update;  
} SensorData;

typedef struct ControlCmd {
  int servo_action;
  char authorized_uid[20];
} ControlCmd;

SensorData incomingData;
ControlCmd outgoingCmd;
bool newData = false;
int last_door_status = -1; 

int current_ui_room = 1;  
int last_unlocked_room = 1; 

int uiState = 0; 
String enteredPin = "";
const String ADMIN_PIN = "1234";
String uidRoom1 = "";
String uidRoom2 = "";

unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;

// 🌟 ฟังก์ชันสร้าง Hash แบบ SHA-256
String calculateHash(String payload) {
  byte shaResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payloadLength = strlen(payload.c_str());
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *) payload.c_str(), payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  String hashStr = "";
  for(int i=0; i<32; i++) {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    hashStr += str;
  }
  return hashStr; 
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  String topicStr = String(topic);
  Serial.printf("MQTT Received: %s -> %s\n", topicStr.c_str(), messageTemp.c_str());

  // 🌟 ใช้ indexOf สแกนหาคำว่า "open" ไม่ว่าเพื่อนจะส่งมาท่าไหนก็รับได้หมด!
  if (messageTemp.indexOf("open") >= 0) {
    int target_room = 0;
    
    if (topicStr == "m5/room1/control") target_room = 1;
    else if (topicStr == "m5/room2/control") target_room = 2;

    if (target_room > 0) {
      last_unlocked_room = target_room; 
      
      outgoingCmd.servo_action = 1;
      String("WEB").toCharArray(outgoingCmd.authorized_uid, sizeof(outgoingCmd.authorized_uid));
      esp_now_send(nodeAddress, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));
      
      CoreS3.Display.fillRect(0, 120, 320, 35, BLACK);
      CoreS3.Display.setCursor(10, 120);
      CoreS3.Display.setTextColor(MAGENTA);
      CoreS3.Display.printf("WEB UNLOCKED: Room %d", target_room); 
    }
  }
}

void drawStatusBar() {
  CoreS3.Display.setTextDatum(top_left);
  CoreS3.Display.setTextSize(2);
  
  if (WiFi.status() != WL_CONNECTED) {
    CoreS3.Display.fillRect(0, 0, 320, 25, RED);
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.drawString("Wi-Fi Lost - Offline", 10, 4); 
  } else if (!mqttClient.connected()) {
    CoreS3.Display.fillRect(0, 0, 320, 25, ORANGE);
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.drawString("MQTT Lost - Retrying...", 10, 4);
  } else {
    CoreS3.Display.fillRect(0, 0, 320, 25, BLACK);
    CoreS3.Display.setTextColor(GREEN);
    CoreS3.Display.drawString("Gateway Ready (Online)", 10, 4);
  }
}

void drawMainMenu() {
  CoreS3.Display.clear();
  drawStatusBar(); 
  
  CoreS3.Display.setTextColor(WHITE);
  CoreS3.Display.setTextDatum(top_left);
  CoreS3.Display.setTextSize(1);
  CoreS3.Display.drawString("R1: " + (uidRoom1=="" ? "Empty" : uidRoom1), 10, 40);
  CoreS3.Display.drawString("R2: " + (uidRoom2=="" ? "Empty" : uidRoom2), 160, 40);

  uint16_t roomColor = (current_ui_room == 1) ? DARKGREEN : MAROON;
  CoreS3.Display.fillRect(10, 70, 300, 40, roomColor);
  
  CoreS3.Display.setTextDatum(middle_center);
  CoreS3.Display.setTextSize(2); 
  CoreS3.Display.drawString("ROOM " + String(current_ui_room), 160, 82); 
  
  CoreS3.Display.setTextSize(1); 
  CoreS3.Display.drawString("(TAP TO CHANGE)", 160, 100);

  CoreS3.Display.setTextSize(2); 
  CoreS3.Display.fillRect(10, 160, 140, 60, BLUE);
  CoreS3.Display.drawString("UNLOCK", 80, 190); 

  CoreS3.Display.fillRect(170, 160, 140, 60, ORANGE);
  CoreS3.Display.drawString("REGISTER", 240, 190);
  CoreS3.Display.setTextDatum(top_left);
}

void drawPinPad() {
  CoreS3.Display.clear();
  drawStatusBar(); 
  
  CoreS3.Display.setTextColor(WHITE);
  CoreS3.Display.setTextDatum(top_center);
  CoreS3.Display.drawString("Enter PIN: " + enteredPin, 160, 40); 
  
  for(int i=0; i<9; i++) {
    int r = i / 3; int c = i % 3;
    int x = c * 80 + 40; int y = r * 50 + 65; 
    CoreS3.Display.drawRect(x, y, 70, 40, WHITE);
    CoreS3.Display.setTextDatum(middle_center);
    CoreS3.Display.drawString(String(i+1), x+35, y+20);
  }
  CoreS3.Display.drawRect(40, 215, 70, 25, RED);
  CoreS3.Display.drawString("C", 75, 227);
  CoreS3.Display.drawRect(120, 215, 70, 25, WHITE);
  CoreS3.Display.drawString("0", 155, 227);
  CoreS3.Display.setTextDatum(top_left);
}

void drawWaitCard() {
  CoreS3.Display.clear();
  drawStatusBar(); 
  
  CoreS3.Display.setTextDatum(middle_center);
  CoreS3.Display.setTextColor(YELLOW);
  CoreS3.Display.drawString("Tap Card to Register", 160, 80);
  CoreS3.Display.drawString("--> ROOM " + String(current_ui_room) + " <--", 160, 120);
  
  CoreS3.Display.fillRect(110, 180, 100, 40, RED);
  CoreS3.Display.setTextColor(WHITE);
  CoreS3.Display.drawString("CANCEL", 160, 200);
  CoreS3.Display.setTextDatum(top_left);
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));
  newData = true; 
}

void setup() {
  CoreS3.begin();
  CoreS3.Display.setTextSize(2);
  
  prefs.begin("rfid_app", false);
  uidRoom1 = prefs.getString("room1", "");
  uidRoom2 = prefs.getString("room2", "");
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); 
  WiFi.begin(ssid, password);
  
  CoreS3.Display.clear();
  CoreS3.Display.setCursor(10, 10);
  CoreS3.Display.print("Connecting WiFi");
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 10) { 
    delay(500); 
    CoreS3.Display.print("."); 
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  }
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, nodeAddress, 6);
  peerInfo.channel = 0; // 🌟 ให้จูนเสาอากาศตาม Wi-Fi อัตโนมัติ
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  uiState = 0;
  drawMainMenu(); 
}

void loop() {
  CoreS3.update();

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiAttempt > 20000) {
      lastWifiAttempt = millis();
      
      CoreS3.Display.fillRect(0, 0, 320, 25, ORANGE);
      CoreS3.Display.setTextColor(WHITE);
      CoreS3.Display.drawString("Scanning Wi-Fi...", 10, 4);

      WiFi.disconnect();
      WiFi.begin(ssid, password);
      
      delay(1000); 
      drawStatusBar(); 
    }
  } else {
    if (!mqttClient.connected()) {
      if (millis() - lastMqttAttempt > 15000) {
        lastMqttAttempt = millis();
        drawStatusBar(); 
        
        if (mqttClient.connect("M5Gateway", mqtt_user, mqtt_pass)) {
          mqttClient.subscribe("m5/room1/control");
          mqttClient.subscribe("m5/room2/control");
          drawStatusBar(); 
        }
      }
    } else {
      mqttClient.loop();
    }
  }

  // ---------------------------------------------------------
  // ระบบสัมผัส
  // ---------------------------------------------------------
  if (CoreS3.Touch.getCount() > 0) {
      auto detail = CoreS3.Touch.getDetail();
      if (detail.wasPressed()) {
          int tx = detail.x; int ty = detail.y;
          
          if (uiState == 0) { 
            if (tx >= 10 && tx <= 310 && ty >= 70 && ty <= 110) {
                current_ui_room = (current_ui_room == 1) ? 2 : 1;
                drawMainMenu();
                delay(200); 
            }
            else if (tx >= 10 && tx <= 150 && ty >= 160 && ty <= 220) {
                last_unlocked_room = current_ui_room; 
                outgoingCmd.servo_action = 1;
                String("").toCharArray(outgoingCmd.authorized_uid, sizeof(outgoingCmd.authorized_uid));
                esp_now_send(nodeAddress, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));
                
                CoreS3.Display.fillRect(0, 120, 320, 35, BLACK);
                CoreS3.Display.setCursor(10, 120);
                CoreS3.Display.setTextColor(GREEN);
                CoreS3.Display.println("UNLOCKING ROOM " + String(current_ui_room) + "..."); 
            }
            else if (tx >= 170 && tx <= 310 && ty >= 160 && ty <= 220) {
                uiState = 1; enteredPin = ""; drawPinPad();
            }
          } 
          else if (uiState == 1) { 
            if (ty >= 65 && ty <= 205) { 
              int col = (tx - 40) / 80; int row = (ty - 65) / 50;
              if(col>=0 && col<=2 && row>=0 && row<=2) {
                enteredPin += String(row*3 + col + 1);
                drawPinPad();
              }
            } else if (ty >= 215 && ty <= 240) { 
              if (tx >= 40 && tx <= 110) { uiState = 0; drawMainMenu(); } 
              if (tx >= 120 && tx <= 190) { enteredPin += "0"; drawPinPad(); } 
            }
            if (enteredPin.length() == 4) {
              if (enteredPin == ADMIN_PIN) { uiState = 3; drawWaitCard(); } 
              else { uiState = 0; drawMainMenu(); }
            }
          }
          else if (uiState == 3) { 
            if (tx >= 110 && tx <= 210 && ty >= 180 && ty <= 220) {
              uiState = 0; drawMainMenu(); 
            }
          }
      }
  }

  // ---------------------------------------------------------
  // ระบบรับข้อมูลและส่ง MQTT (อัปเดต Hash แล้ว)
  // ---------------------------------------------------------
  if (newData) {
    newData = false;
    
    time_t now; time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeString[30];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    char jsonBuffer[500]; // ขยายขนาดกล่องให้พอใส่ Hash
    
    String rName = "room" + String(current_ui_room);
    String myHash = calculateHash(rName + SECRET_KEY); // สร้าง Hash รอไว้เลย

    if (incomingData.is_sensor_update) {
        String topicPower = "m5/" + rName + "/power"; 
        float power_watt = incomingData.current_value * 5.0;
        
        snprintf(jsonBuffer, sizeof(jsonBuffer), 
          "{\"room\":\"%s\",\"current_amp\":%.2f,\"power_watt\":%.2f,\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
          rName.c_str(), incomingData.current_value, power_watt, timeString, myHash.c_str());
          
        if (mqttClient.connected()) mqttClient.publish(topicPower.c_str(), jsonBuffer);
        
        CoreS3.Display.fillRect(0, 225, 320, 15, BLACK);
        CoreS3.Display.setCursor(10, 225);
        CoreS3.Display.setTextSize(1);
        CoreS3.Display.setTextColor(CYAN);
        CoreS3.Display.printf("Load: 5.00V | %.2fA | %.2fW", incomingData.current_value, power_watt);
        CoreS3.Display.setTextSize(2); 
        
        return; 
    }

    if (uiState == 3 && incomingData.rfid_detected && incomingData.door_status == 0) {
        String newUid = String(incomingData.rfid_uid);
        String topicName = "m5/" + rName + "/doorstatus"; 

        if (current_ui_room == 1) {
            uidRoom1 = newUid; prefs.putString("room1", uidRoom1); 
        } else {
            uidRoom2 = newUid; prefs.putString("room2", uidRoom2); 
        }
        uiState = 0; drawMainMenu(); 
        
        snprintf(jsonBuffer, sizeof(jsonBuffer), 
          "{\"Register\":true,\"room\":\"%s\",\"status\":\"saved\",\"M5\":false,\"RFID\":\"%s\",\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
          rName.c_str(), newUid.c_str(), timeString, myHash.c_str());
        if (mqttClient.connected()) mqttClient.publish(topicName.c_str(), jsonBuffer);
        return; 
    }

    if (uiState == 0 && incomingData.rfid_detected && incomingData.door_status == 0) {
        String tappedUid = String(incomingData.rfid_uid);
        int roomMatch = 0;
        
        if (tappedUid == uidRoom1) roomMatch = 1;
        else if (tappedUid == uidRoom2) roomMatch = 2;

        if (roomMatch > 0) {
            last_unlocked_room = roomMatch; 
            outgoingCmd.servo_action = 1;
            tappedUid.toCharArray(outgoingCmd.authorized_uid, sizeof(outgoingCmd.authorized_uid));
            esp_now_send(nodeAddress, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));
            
            CoreS3.Display.fillRect(0, 120, 320, 35, BLACK);
            CoreS3.Display.setCursor(10, 120);
            CoreS3.Display.setTextColor(GREEN);
            CoreS3.Display.printf("GRANTED: Room %d", roomMatch);
        } else {
            CoreS3.Display.fillRect(0, 120, 320, 35, BLACK);
            CoreS3.Display.setCursor(10, 120);
            CoreS3.Display.setTextColor(RED);
            CoreS3.Display.printf("DENIED: Unknown Card");

            String topicName = "m5/" + rName + "/doorstatus";
            snprintf(jsonBuffer, sizeof(jsonBuffer), 
              "{\"Register\":false,\"room\":\"%s\",\"status\":\"denied\",\"M5\":false,\"RFID\":\"%s\",\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
              rName.c_str(), tappedUid.c_str(), timeString, myHash.c_str());
            if (mqttClient.connected()) mqttClient.publish(topicName.c_str(), jsonBuffer);
        }
        return; 
    }

    if (uiState == 0 && incomingData.door_status != last_door_status && !incomingData.is_sensor_update) {
      rName = "room" + String(last_unlocked_room); // อัปเดตชื่อห้องเป็นห้องที่เพิ่งสั่งปลดล็อก
      myHash = calculateHash(rName + SECRET_KEY);  // คำนวณ Hash ใหม่ให้ตรงกัน
      String topicName = "m5/" + rName + "/doorstatus"; 

      if (incomingData.door_status == 1) { 
        if (incomingData.rfid_detected) {
          snprintf(jsonBuffer, sizeof(jsonBuffer), 
            "{\"Register\":false,\"room\":\"%s\",\"status\":\"open\",\"M5\":false,\"RFID\":\"%s\",\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
            rName.c_str(), incomingData.rfid_uid, timeString, myHash.c_str());
        } else {
          snprintf(jsonBuffer, sizeof(jsonBuffer), 
            "{\"Register\":false,\"room\":\"%s\",\"status\":\"open\",\"M5\":true,\"RFID\":null,\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
            rName.c_str(), timeString, myHash.c_str());
        }
      } else { 
        snprintf(jsonBuffer, sizeof(jsonBuffer), 
          "{\"Register\":false,\"room\":\"%s\",\"status\":\"closed\",\"M5\":false,\"RFID\":null,\"Timestamp\":\"%s\",\"hash\":\"%s\"}", 
          rName.c_str(), timeString, myHash.c_str());
        
        CoreS3.Display.fillRect(0, 120, 320, 35, BLACK);
        CoreS3.Display.setCursor(10, 120);
        CoreS3.Display.setTextColor(WHITE);
        CoreS3.Display.println("Door LOCKED"); 
      }
      
      if (mqttClient.connected()) mqttClient.publish(topicName.c_str(), jsonBuffer);
      last_door_status = incomingData.door_status;
    }
  }
}