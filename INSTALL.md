# 🔧 QUICK START GUIDE - การติดตั้ง Lumina Dashboard

## 🐳 วิธีที่ 1: ใช้ Docker (ง่ายที่สุด ⭐)

### Step 1: ดาวน์โหลด Project
```bash
git clone <your-repo-url>
cd projecthotel
```

### Step 2: ตั้งค่า Environment Variables
```bash
# Copy template
cp .env.example .env

# เปิดแก้ไข (ไม่บังคับ ใช้ default ได้)
nano .env
```

**คำแนะนำ**: แก้ไข `SECRET_KEY` และ password ให้ปลอดภัย

### Step 3: เริ่มต้น Docker Services
```bash
# เริ่มต้น services ทั้งหมด (ครั้งแรกจะดาวน์โหลด images)
docker compose up -d

# เช็คสถานะ (ควร "Up" ทั้งหมด)
docker compose ps

# ดู logs
docker compose logs -f
```

### Step 4: เข้า Dashboard
- เปิด http://localhost:3000 ในเบราว์เซอร์
- Login:
  - Username: `admin`
  - Password: `admin123`

### ✅ เสร็จแล้ว! 🎉

---

## 🖥️ วิธีที่ 2: ติดตั้งแบบ Manual (บน Linux/Mac)

### Step 1: ติดตั้ง Prerequisites

#### 1.1 ติดตั้ง Node.js
```bash
# Ubuntu/Debian
curl -sL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Mac
brew install node
```

#### 1.2 ติดตั้ง MySQL
```bash
# Ubuntu/Debian
sudo apt-get install mysql-server

# Mac
brew install mysql@8.0
brew services start mysql@8.0
```

#### 1.3 ติดตั้ง MQTT Broker
```bash
# Ubuntu/Debian
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Mac
brew install mosquitto
brew services start mosquitto
```

### Step 2: Setup Database
```bash
# เข้า MySQL shell
mysql -u root

# รัน SQL เหล่านี้:
CREATE DATABASE hotel_db;
USE hotel_db;

CREATE TABLE IF NOT EXISTS door_event (
  id INT AUTO_INCREMENT PRIMARY KEY,
  room VARCHAR(20),
  status VARCHAR(20),
  source VARCHAR(50),
  rfid_uid VARCHAR(50),
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS power_consumption (
  id INT AUTO_INCREMENT PRIMARY KEY,
  room VARCHAR(20),
  current_amp FLOAT,
  power_watt FLOAT,
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS bookings (
  id INT AUTO_INCREMENT PRIMARY KEY,
  guest_name VARCHAR(100),
  room_no VARCHAR(20),
  checkin_date DATE,
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS rfid_register (
  id INT AUTO_INCREMENT PRIMARY KEY,
  room VARCHAR(20),
  rfid_uid VARCHAR(50),
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

# ออก MySQL
EXIT;
```

### Step 3: ตั้งค่า Node.js Server
```bash
# ไปยัง project directory
cd projecthotel/hotel-dashboard

# ติดตั้ง dependencies
npm install

# Copy .env
cp ../.env.example ../.env
```

### Step 4: แก้ไข .env
```bash
nano ../.env
```

เปลี่ยนค่าเหล่านี้:
```env
DB_HOST=localhost          # (ไม่ใช่ mysql)
MQTT_HOST=localhost        # (ไม่ใช่ mqtt)
```

### Step 5: เริ่มต้น Server
```bash
# จากในโฟลเดอร์ hotel-dashboard
npm start

# หรือใช้ nodemon สำหรับ development
npm install -D nodemon
npx nodemon server.js
```

### Step 6: เข้า Dashboard
- เปิด http://localhost:3000
- Login ด้วย admin credentials

---

## 📱 การ Upload Firmware ไปยัง M5 Device

### Step 1: ดาวน์โหลด Arduino IDE
- ไปที่ https://www.arduino.cc/en/software
- ติดตั้งตามเขต OS

### Step 2: ตั้งค่า M5Stack Board
```
Arduino IDE ➜ File ➜ Preferences

URL ที่ต้องเพิ่ม:
https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json

OU File ➜ Boards Manager ➜ ค้นหา "M5Stack" ➜ Install
```

### Step 3: เตรียม Firmware
```
1. เปิด Arduino IDE
2. File ➜ Open ➜ m5 and esp32/m5.ino
3. เพิ่มไลบรารี่ที่ต้อง:
   Sketch ➜ Include Library ➜ Manage Libraries
   - M5CoreS3
   - PubSubClient
   - mbedtls
```

### Step 4: สร้างไฟล์ WiFi Config
สร้างไฟล์ `m5 and esp32/secret.h`:
```cpp
#define ssid "YOUR_WIFI_NAME"
#define password "YOUR_WIFI_PASSWORD"
#define mqtt_server "192.168.1.100"  // IP ของ Server
#define mqtt_port 1883
#define mqtt_user "mosquitto"
#define mqtt_pass "mosquitto"
```

### Step 5: Upload
```
1. ต่อ M5 ด้วย USB Cable
2. Tools ➜ Select Board ➜ M5Stack CoreS3
3. Tools ➜ Port ➜ /dev/ttyUSB0 (หรือ COM port บน Windows)
4. Sketch ➜ Upload
5. เช็ค Serial Monitor (115200 baud) ว่า connect ได้หรือไม่
```

### Step 6: ทดสอบ M5 Device
ดู Serial Monitor:
```
[INFO] Connecting WiFi...
[OK] WiFi Connected!
[INFO] Connecting MQTT...
[OK] MQTT Connected!
🌐 Gateway Ready (Online)
```

---

## ⚠️ Troubleshooting

### ❌ Docker Container Crashes
```bash
# ดู error
docker compose logs hotel_dashboard

# Rebuild images
docker compose down
docker compose build --no-cache
docker compose up -d
```

### ❌ Cannot Connect to MQTT
```bash
# เช็ค MQTT บ่านกำลัง run
docker compose logs hotel_mqtt

# ทดสอบ connection
mosquitto_pub -h localhost -p 1883 -t "test" -m "hello"
```

### ❌ MySQL Connection Error
```bash
# ตรวจสอบ password ถูกต้อง
# เช็ค DB_HOST ใน .env

# ลองเข้า MySQL โดยตรง
mysql -h localhost -u root -p

# ถ้าลืมรหัสผ่าน (Docker):
docker compose exec mysql mysql -u root  # ไม่มี password
```

### ❌ Dashboard ยังไม่ load
```bash
# รอให้ server พร้อม (ครั้งแรกใช้เวลา 30-60 วินาที)
docker compose logs -f hotel_dashboard | grep "Server running"

# Refresh browser หลังจาก "Server running" ขึ้น
```

### ❌ M5 ไม่ Connect WiFi
```
1. เช็ค WiFi name/password ใน secret.h
2. ดู Serial Monitor (Baud: 115200)
3. ตรวจสอบ M5 อยู่ในระหว่าง 2.4GHz WiFi (ไม่ใช่ 5GHz)
```

---

## 🚀 คำสั่ง Useful

### Docker Commands
```bash
# เดินทุกอย่าง
docker compose up -d

# ปรับปรุง logs
docker compose logs -f [service-name]

# Restart บริการ
docker compose restart hotel_dashboard

# Stop ทั้งหมด
docker compose down

# ลบข้อมูล (BE CAREFUL!)
docker compose down -v
```

### Database Commands
```bash
# เข้า MySQL
docker compose exec mysql mysql -u root -photel123 hotel_db

# ล้าง Door Events
TRUNCATE TABLE door_event;

# ดู Latest Events
SELECT * FROM door_event ORDER BY timestamp DESC LIMIT 10;
```

### MQTT Commands
```bash
# Subscribe topic
docker compose exec mqtt mosquitto_sub -h localhost -t "m5/+/doorstatus"

# Publish test message
docker compose exec mqtt mosquitto_pub -h localhost -t "test/msg" -m "hello"
```

---

## ✅ Verification Checklist

- [ ] Docker Compose up ได้โดยไม่ error
- [ ] ทั้ง 3 containers รัน (hotel_dashboard, mysql, mqtt)
- [ ] http://localhost:3000 เข้าได้
- [ ] Login ได้ด้วย admin/admin123
- [ ] MySQL tables มีอยู่ 4 ตาราง
- [ ] M5 Device upload firmware ได้
- [ ] Serial Monitor แสดง "MQTT Connected"
- [ ] Dashboard แสดง Room 1 & Room 2
- [ ] M5 กดปุ่ม unlock → Dashboard แสดง updated เกิดขึ้น

---

## 📞 ไม่สามารถแก้ไขได้?

ดู logs ด้วยคำสั่ง:
```bash
# All services
docker compose logs

# Specific service
docker compose logs hotel_dashboard   # Dashboard
docker compose logs hotel_database    # MySQL
docker compose logs hotel_mqtt        # MQTT

# Follow logs (real-time)
docker compose logs -f
```

และบอกสิ่งต่อไปนี้:
- Error message ที่ทำให้คุณติ่ง
- ผลลัพธ์ของ `docker compose ps`
- ผลลัพธ์ของคำสั่ง logs

---

**🎉 ขอบคุณที่ใช้ Lumina Dashboard!**
