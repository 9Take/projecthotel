# 🏨 Lumina Hotel IoT Dashboard System

ระบบจัดการห้องพักแบบอัจฉริยะสำหรับโรงแรม ที่ประกอบด้วย Smart Door Lock, Power Monitoring, RFID Card Reader, และ Real-time Dashboard

---

## 📋 สารบัญ
- [ภาพรวมระบบ](#-ภาพรวมระบบ)
- [สถาปัตยกรรม](#-สถาปัตยกรรม)
- [ความต้องการของระบบ](#-ความต้องการของระบบ)
- [การติดตั้ง](#-การติดตั้ง)
- [การกำหนดค่า](#-การกำหนดค่า)
- [การใช้งาน](#-การใช้งาน)
- [คุณสมบัติ](#-คุณสมบัติ)
- [Troubleshooting](#-troubleshooting)

---

## 🎯 ภาพรวมระบบ

**Lumina** คือระบบจัดการห้องพักแบบรวมศูนย์ที่ช่วยให้โรงแรมสามารถ:

✅ **Access Control** - ควบคุมการเปิดปิดประตูห้องพักได้จากหน้า Dashboard หรือบัตร RFID
✅ **Real-time Monitoring** - ติดตามสถานะห้องพัก (ปิด/เปิด) ได้แบบ real-time
✅ **Power Consumption** - วัดการใช้พลังงานในแต่ละห้องพัก
✅ **Guest Booking** - จัดการการจองห้องพักของแขก
✅ **Audit Log** - บันทึกประวัติการเข้าออกห้องพักอย่างคงทั่ว

---

## 🏗 สถาปัตยกรรม

```
┌─────────────────────────────────────────────────────────────┐
│                     Admin Dashboard (Web)                    │
│                  http://localhost:3000                       │
│  - Login: admin/admin123 (หรือตามค่า .env)                   │
└────────────────┬────────────────────────────────────────────┘
                 │ Socket.io (Real-time)
                 ↓
┌─────────────────────────────────────────────────────────────┐
│         Node.js Server (Hotel Dashboard)                     │
│         - Port 3000                                          │
│         - WebSocket handling                                 │
│         - MQTT client                                        │
│         - Database queries                                   │
└─┬──────────────────┬──────────────────────┬─────────────────┘
  │                  │                      │
  ↓ MQTT (1883)      ↓ SQL (3306)          ↓ CSV Logging
┌──────────────┐  ┌──────────────┐    ┌────────────────┐
│ MQTT Broker  │  │ MySQL DB     │    │ Data Logging   │
│ (Eclipse     │  │ - door_event │    │ (hotel_data    │
│  Mosquitto)  │  │ - bookings   │    │  _log.csv)     │
└──────┬───────┘  └──────────────┘    └────────────────┘
       │
       ↓ MQTT Topics
  ┌────────────────┐
  │ M5 Gateway     │
  │ (M5CoreS3)     │
  │ - UI Control   │
  │ - Door Status  │
  │ - RFID Reader  │
  └────┬────┬─────┘
       │    │
       ↓    ↓
  ┌─────────────────────┐
  │ ESP32 Node          │
  │ - Door Lock         │
  │ - Sensor Reader     │
  │ - Power Monitor     │
  │ (ACS712)            │
  └─────────────────────┘
```

---

## 💻 ความต้องการของระบบ

### Server
- **Docker** & **Docker Compose** (สำหรับเทพใหญ่)
- หรือติดตั้ง:
  - **Node.js** v14+ (สำหรับ Dashboard Server)
  - **MySQL** 5.7+ (Database)
  - **MQTT Broker** (Eclipse Mosquitto)

### Hardware
- **M5CoreS3** - Gateway หลัก (รับคำสั่งจาก Dashboard, แสดง UI)
- **ESP32** - Node device (ควบคุมประตู, อ่านเซนเซอร์)
- **RFID Reader** - สำหรับแตะบัตร RFID
- **Servo Motor** - สำหรับควบคุมการปลดล็อกประตู
- **ACS712** - Current Sensor (วัดการใช้ไฟ)

---

## 🔧 การติดตั้ง

### วิธีที่ 1: ใช้ Docker (แนะนำ) ⭐

#### 1. Clone Project
```bash
git clone <your-repo-url>
cd projecthotel
```

#### 2. ตั้งค่าไฟล์ `.env`
```bash
cp .env.example .env  # ถ้ามี หรือสร้างใหม่
```

แก้ไขไฟล์ `.env`:
```env
# Database
DB_HOST=mysql
DB_USER=root
DB_PASSWORD=hotel123
DB_NAME=hotel_db

# MQTT
MQTT_HOST=mqtt
MQTT_USER=mosquitto
MQTT_PASSWORD=mosquitto
MQTT_PORT=1883

# Admin Credentials
ADMIN_USER=admin
ADMIN_PASS=admin123
GUEST_USER=guest
GUEST_PASS=guest123

# Security
SECRET_KEY=LUMINA2026
SESSION_SECRET=your-secret-key
```

#### 3. รัน Docker Compose
```bash
docker compose up -d
```

เช็คสถานะ:
```bash
docker compose ps
```

#### 4. เข้าใจ Dashboard
- Admin: http://localhost:3000
- Username: `admin`
- Password: `admin123`

---

### วิธีที่ 2: ติดตั้งแบบ Manual

#### 1. ติดตั้ง MySQL
```bash
sudo apt-get install mysql-server
mysql -u root -p  # สร้าง database hotel_db
```

#### 2. ติดตั้ง MQTT Broker
```bash
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl start mosquitto
```

#### 3. ติดตั้ง Node.js Dependencies
```bash
cd hotel-dashboard
npm install
```

#### 4. สตาร์ท Server
```bash
npm start
# หรือ
node server.js
```

---

## ⚙️ การกำหนดค่า

### 1. Setup Database Tables
Tables จะถูกสร้างอัตโนมัติเมื่อ Server เริ่มต้น

การดูข้อมูล:
```bash
# เข้า MySQL
docker compose exec mysql mysql -u root -p

# Query
USE hotel_db;
SHOW TABLES;
SELECT * FROM door_event;
```

### 2. ตั้งค่า M5 Device

#### Upload Firmware ไปยัง M5CoreS3
1. เปิด **Arduino IDE**
2. ไป `File > Open` และเลือก `m5 and esp32/m5.ino`
3. ตั้ง Board: **M5Stack CoreS3**
4. เลือก Port ที่ connected
5. คลิก **Upload**

#### Setup WiFi & MQTT ใน M5
สร้างไฟล์ `secret.h`:
```cpp
#define ssid "YOUR_WIFI_SSID"
#define password "YOUR_WIFI_PASSWORD"
#define mqtt_server "YOUR_MQTT_SERVER"
#define mqtt_port 1883
#define mqtt_user "mosquitto"
#define mqtt_pass "mosquitto"
```

### 3. ตั้งค่า ESP32 Node

Upload ไฟล์ `m5 and esp32/esp32.ino` ไปยัง ESP32 ด้วยวิธีเดียวกัน

---

## 📱 การใช้งาน

### หน้าแรก (Login)
```
┌─────────────────────────┐
│   LUMINA Dashboard      │
├─────────────────────────┤
│                         │
│  Username: [           ]│
│  Password: [           ]│
│                         │
│    [  Login  ]          │
└─────────────────────────┘
```

### Admin Dashboard

#### 1. **Live Sensors** Tab
- ดูการใช้ไฟฟ้าแบบ Real-time (กราฟ)
- ดูประวัติการเข้าออกห้องพัก

#### 2. **Access Control** Tab
- ดูสถานะประตู (ปิด/เปิด)
- สั่ง Unlock/Lock ประตูได้ทันที
- Source แสดง:
  - 🌐 **WEB** = สั่งจาก Dashboard
  - 📺 **M5 Screen** = สั่งจากหน้าจอ M5
  - 🏷️ **RFID Card** = แตะบัตร RFID

#### 3. **Bookings** Tab
- ดูรายการจองห้องพักทั้งหมด
- Check-in date ของแขก

### Guest Portal
- Guest สามารถใช้รหัส PIN เพื่อเข้าไปใช้งานได้

---

## 🌟 คุณสมบัติ

### ✅ Access Control
- ✓ Unlock/Lock จาก Dashboard
- ✓ Unlock โดยการแตะบัตร RFID
- ✓ Unlock โดยปุ่มที่ M5 Screen
- ✓ Auto-lock หลังเปิดนาน 30 วินาที

### ✅ Security
- ✓ SHA-256 Hash Verification ของทุก Payload
- ✓ Session-based Authentication
- ✓ สำหรับ Admin และ Guest แยกกัน

### ✅ Logging & Audit
- ✓ บันทึก Event ทั้งหมดใน Database
- ✓ Export เป็น CSV (`hotel_data_log.csv`)
- ✓ Timestamp ที่ถูกต้อง (Bangkok Time UTC+7)

### ✅ Real-time Updates
- ✓ Socket.io Push Notification
- ✓ Live Power Graph
- ✓ Instant Door Status

### ✅ Power Monitoring
- ✓ วัด Current (A) และ Watt (W)
- ✓ กราฟแสดง 10 ล่าสุด

---

## 🔍 API Endpoints

### WebSocket Events (Socket.io)

**Client → Server:**
```js
// Request ประวัติการเข้าออก
socket.emit('request_history', 'room1');

// Request ประวัติการใช้ไฟ
socket.emit('request_power_history', 'room1');

// Request รายการจอง
socket.emit('request_bookings');

// สั่ง Unlock/Lock
socket.emit('send_control', {
  room: 'room1',
  command: 'open' // or 'close'
});
```

**Server → Client:**
```js
// อัปเดตสถานะประตู Real-time
socket.on('door_update', (data) => {
  console.log(data.status); // 'open' or 'closed'
  console.log(data.source);  // 'WEB', 'M5 Screen', 'RFID Card'
});

// อัปเดตการใช้ไฟ Real-time
socket.on('power_update', (data) => {
  console.log(data.current_amp); // เป็น A
  console.log(data.power_watt);  // เป็น W
});
```

### MQTT Topics

```
m5/room1/doorstatus    → ส่ง JSON สถานะประตู
m5/room1/power         → ส่ง JSON ข้อมูลการใช้ไฟ
m5/room1/control       → รับคำสั่ง open/close จาก Dashboard
```

---

## 📊 Database Schema

### `door_event` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| room | VARCHAR(20) | ชื่อห้อง (room1, room2) |
| status | VARCHAR(20) | สถานะ (open, closed) |
| source | VARCHAR(50) | แหล่งที่มา (WEB, M5 Screen, RFID Card) |
| rfid_uid | VARCHAR(50) | UID บัตร RFID |
| timestamp | TIMESTAMP | วันเวลา |

### `power_consumption` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| room | VARCHAR(20) | ชื่อห้อง |
| current_amp | FLOAT | กระแสไฟ (A) |
| power_watt | FLOAT | กำลังไฟ (W) |
| timestamp | TIMESTAMP | วันเวลา |

### `bookings` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| guest_name | VARCHAR(100) | ชื่อแขก |
| room_no | VARCHAR(20) | เลขห้อง |
| checkin_date | DATE | วันเข้าพัก |
| timestamp | TIMESTAMP | วันเวลาจอง |

---

## 🐛 Troubleshooting

### ⚠️ Dashboard ไม่เข้าได้
```bash
# เช็ค server ว่า run อยู่หรือไม่
docker compose ps

# ดู logs
docker compose logs hotel_dashboard

# Restart
docker compose restart hotel_dashboard
```

### ⚠️ MQTT ไม่ connect
```bash
# เช็ค MQTT broker
docker compose logs hotel_mqtt

# ทดสอบ MQTT
mosquitto_pub -h localhost -p 1883 -t "test" -m "hello"
mosquitto_sub -h localhost -p 1883 -t "test"
```

### ⚠️ M5 Device ไม่ connect
- เช็ค WiFi SSID/Password ใน `secret.h`
- เช็ค MQTT Server address ถูกต้อง
- ดู Serial Monitor (Baud 115200)

### ⚠️ เวลาไม่ถูกต้อง
Server ตั้ง timezone Bangkok (UTC+7) อัตโนมัติ แต่ M5 device ยังคงใช้เวลาจากตัวเอง ซึ่งอาจผิด ให้ใช้เวลาจาก Server (Dashboard)

### ⚠️ ประตูไม่ปิดอัตโนมัติ
- เช็ค Delay ใน M5 firmware
- เช็ค Servo Motor ว่า calibrate ถูกต้องหรือไม่

---

## 📝 File Structure

```
projecthotel/
├── README.md                      # Document นี้
├── docker-compose.yaml            # Docker services
├── .env                           # Configuration
├── .dockerignore & .gitignore     # Git ignore
│
├── hotel-dashboard/
│   ├── package.json               # Node.js dependencies
│   ├── server.js                  # Main server file
│   ├── dockerfile                 # Docker image
│   │
│   └── public/
│       ├── dashboard.html         # Admin dashboard
│       ├── guest.html             # Guest portal
│       └── login.html             # Login page
│
├── m5 and esp32/
│   ├── m5.ino                     # M5CoreS3 firmware
│   ├── esp32.ino                  # ESP32 Node firmware
│   └── secret.h                   # WiFi & MQTT config
│
└── mosquitto/                     # MQTT config & data
    └── config/mosquitto.conf      # MQTT broker config
```

---

## 🔐 Security Best Practices

1. **เปลี่ยน Default Credentials**
   ```env
   ADMIN_USER=your-admin-username
   ADMIN_PASS=your-secure-password
   SECRET_KEY=change-this-key
   SESSION_SECRET=random-secret-key
   ```

2. **ตั้ง Firewall**
   ```bash
   sudo ufw allow 3000/tcp     # Dashboard
   sudo ufw allow 1883/tcp     # MQTT
   sudo ufw allow 3306/tcp     # MySQL (ถ้ากำหนด)
   ```

3. **ใช้ HTTPS** (ถ้าเป็น Production)
   - ใช้ SSL certificate
   - ตั้ง reverse proxy (Nginx)

4. **Hash Verification**
   - ทุก Payload ต้อง SHA-256 Hash ที่ตรงกับ `SECRET_KEY`
   - Server จะ verify ของทุก message

---

## 🚀 Next Steps

- [ ] Deploy to Cloud Server
- [ ] Setup SSL/HTTPS
- [ ] Implement OTA Update สำหรับ M5 Device
- [ ] Add Email Notifications
- [ ] Backup Database Schedule
- [ ] Monitor System Performance

---

## 📞 Support & Documentation

- **MQTT Broker**: [Eclipse Mosquitto](https://mosquitto.org/)
- **M5Stack Docs**: [M5Stack Official](https://docs.m5stack.com/)
- **Socket.io Docs**: [Socket.io](https://socket.io/)
- **Node.js Docs**: [Node.js](https://nodejs.org/)

---

## 📄 License

MIT License - ใช้อย่างไรตามใจได้

---

**Last Updated:** 31 March 2026
**Version:** 1.0.0
**Status:** ✅ Production Ready
