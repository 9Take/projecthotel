#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// *** MAC Address ของ M5Stack Gateway ***
uint8_t gatewayAddress[] = {0x48, 0x27, 0xE2, 0x66, 0xD8, 0x30}; 

#define SERVO_PIN 22 
#define SS_PIN    5  
#define RST_PIN   27 
#define ACS_PIN   34 

Servo myServo;
MFRC522 mfrc522(SS_PIN, RST_PIN);

const float sensitivity = 0.185; 
float vZeroPin = 0.0;            

// โครงสร้างข้อมูล (อัปเดตตรงกับ M5)
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

SensorData outgoingData;
ControlCmd incomingCmd;
bool manualOpenTriggered = false; 
String approvedUID = ""; 

unsigned long lastPowerSendTime = 0;

float getACS712Current() {
  long sumADC = 0;
  for(int i = 0; i < 500; i++) {
    sumADC += analogRead(ACS_PIN);
    delay(1); 
  }
  float avgRawADC = sumADC / 500.0;
  
  float vPin = (avgRawADC / 4095.0) * 3.3;
  float deltaV_Pin = vPin - vZeroPin;
  
  if (abs(deltaV_Pin) < 0.005) {
      return 0.00;
  }

  float deltaV_Sensor = deltaV_Pin * 1.5;
  float current = abs(deltaV_Sensor) / sensitivity; 

  float correction_factor = 1.00; 
  current = current * correction_factor;
  
  return current;
}

void openDoor(String uid = "") {
  Serial.println(">>> Opening Door...");
  myServo.write(165);  
  
  outgoingData.is_sensor_update = false; 
  outgoingData.node_id = 1;
  outgoingData.door_status = 1;      
  
  if (uid != "") {
    outgoingData.rfid_detected = true;
    uid.toCharArray(outgoingData.rfid_uid, sizeof(outgoingData.rfid_uid));
  } else {
    outgoingData.rfid_detected = false;
    String("").toCharArray(outgoingData.rfid_uid, sizeof(outgoingData.rfid_uid));
  }
  
  esp_now_send(gatewayAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
  delay(3000); 
  Serial.println(">>> Door Closed.");
  myServo.write(0);   

  outgoingData.door_status = 0; 
  outgoingData.rfid_detected = false; 
  String("").toCharArray(outgoingData.rfid_uid, sizeof(outgoingData.rfid_uid)); 
  esp_now_send(gatewayAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  memcpy(&incomingCmd, data, sizeof(incomingCmd));
  if (incomingCmd.servo_action == 1) {
    manualOpenTriggered = true; 
    approvedUID = String(incomingCmd.authorized_uid);
  }
}

void setup() {
  Serial.begin(115200);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0); 

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Ready!");

  // เปิด Wi-Fi ก่อน
  WiFi.mode(WIFI_STA);
  
  // 🌟 1. ให้สแกนหาว่าเน็ต OFA ปล่อยคลื่นอยู่ช่องไหน
  int myChannel = 1; 
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == "OFA") {
      myChannel = WiFi.channel(i);
      break;
    }
  }
  Serial.printf("Found OFA on Channel: %d\n", myChannel);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(myChannel, WIFI_SECOND_CHAN_NONE); // 🌟 2. จูนเสาอากาศตาม
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = myChannel; // 🌟 3. ตั้งค่า Peer ให้ตรงกัน
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Calibrate หลังจากไฟนิ่งแล้ว
  Serial.print("Calibrating ACS712 Zero Point... ");
  delay(1000); 
  long zeroSum = 0;
  for(int i = 0; i < 500; i++) {
    zeroSum += analogRead(ACS_PIN);
    delay(2);
  }
  vZeroPin = ((zeroSum / 500.0) / 4095.0) * 3.3;
  Serial.printf("Done! vZeroPin = %.3f V\n", vZeroPin);
}

void loop() {
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    mfrc522.PCD_Init(); 
    delay(50);
  }

  // ส่งข้อมูลค่าไฟทุก 5 วินาที
  if (millis() - lastPowerSendTime >= 5000) {
    lastPowerSendTime = millis();
    
    outgoingData.is_sensor_update = true; 
    outgoingData.current_value = getACS712Current(); 
    outgoingData.door_status = myServo.read() == 90 ? 1 : 0; 
    outgoingData.rfid_detected = false;
    
    Serial.printf("Current: %.3fA\n", outgoingData.current_value);
    esp_now_send(gatewayAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
  }

  if (manualOpenTriggered) {
    manualOpenTriggered = false; 
    openDoor(approvedUID); 
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase(); 
    
    outgoingData.is_sensor_update = false; 
    outgoingData.node_id = 1;
    outgoingData.door_status = 0; 
    outgoingData.rfid_detected = true;
    uidString.toCharArray(outgoingData.rfid_uid, sizeof(outgoingData.rfid_uid));
    esp_now_send(gatewayAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
    
    mfrc522.PICC_HaltA();
    delay(1000); 
  }
}