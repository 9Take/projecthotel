# 🏨 Lumina Hotel IoT Dashboard System

A Smart Room Management System for Hotels with Smart Door Lock, Power Monitoring, RFID Card Reader, and Real-time Web Dashboard powered by IoT Gateway.

---

## 📋 Table of Contents
- [System Overview](#-system-overview)
- [Architecture](#-architecture)
- [System Requirements](#-system-requirements)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [Features](#-features)
- [Troubleshooting](#-troubleshooting)
- [Team & Credits](#-team--credits)

---

## 🎯 System Overview

**Lumina** is a centralized room management system that enables hotels to:

✅ **Access Control** - Control door lock/unlock from Dashboard or RFID card
✅ **Real-time Monitoring** - Track room status (open/closed) in real-time
✅ **Power Consumption** - Monitor electrical usage in each room
✅ **Guest Booking** - Manage guest room reservations
✅ **Audit Trail** - Maintain permanent records of all access events

---

## 🏗 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Admin Dashboard (Web)                    │
│                  http://localhost:3000                       │
│  - Login: admin/admin123 (customizable via .env)             │
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
  ↓ MQTT (1883)      ↓ SQL (3306)          ↓ CSV Export
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

## 💻 System Requirements

### Server
- **Docker** & **Docker Compose** (recommended)
- Or manually install:
  - **Node.js** v14+ (for Dashboard Server)
  - **MySQL** 5.7+ (Database)
  - **MQTT Broker** (Eclipse Mosquitto)

### Hardware
- **M5CoreS3** - Main Gateway (receives commands from Dashboard, displays UI)
- **ESP32** - Node device (controls door, reads sensors)
- **RFID Reader** - For card tap authentication
- **Servo Motor** - Controls door lock mechanism
- **ACS712** - Current Sensor (measures power usage)

---

## 🔧 Installation

### Method 1: Using Docker (Recommended) ⭐

#### Step 1: Clone Project
```bash
git clone <your-repo-url>
cd projecthotel
```

#### Step 2: Configure Environment Variables
```bash
cp .env.example .env
```

Edit `.env` file (optional - defaults work):
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

#### Step 3: Start Docker Services
```bash
# Start all services (first time downloads images)
docker compose up -d

# Check status (all should show "Up")
docker compose ps

# View logs
docker compose logs -f
```

#### Step 4: Access Dashboard
- Open http://localhost:3000 in browser
- Login:
  - Username: `admin`
  - Password: `admin123`

### ✅ Done! 🎉

---

### Method 2: Manual Installation (Linux/Mac)

#### Step 1: Install Prerequisites

##### 1.1 Install Node.js
```bash
# Ubuntu/Debian
curl -sL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Mac
brew install node
```

##### 1.2 Install MySQL
```bash
# Ubuntu/Debian
sudo apt-get install mysql-server

# Mac
brew install mysql@8.0
brew services start mysql@8.0
```

##### 1.3 Install MQTT Broker
```bash
# Ubuntu/Debian
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Mac
brew install mosquitto
brew services start mosquitto
```

#### Step 2: Setup Database
```bash
# Enter MySQL shell
mysql -u root

# Run these SQL commands:
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

# Exit MySQL
EXIT;
```

#### Step 3: Configure Node.js Server
```bash
# Navigate to project
cd projecthotel/hotel-dashboard

# Install dependencies
npm install

# Copy environment file
cp ../.env.example ../.env
```

#### Step 4: Edit .env
```bash
nano ../.env
```

Change these values:
```env
DB_HOST=localhost          # (not 'mysql')
MQTT_HOST=localhost        # (not 'mqtt')
```

#### Step 5: Start Server
```bash
# From hotel-dashboard directory
npm start

# Or use nodemon for development
npm install -D nodemon
npx nodemon server.js
```

#### Step 6: Access Dashboard
- Open http://localhost:3000
- Login with admin credentials

---

## ⚙️ Configuration

### 1. Setup Database Tables
Tables are created automatically when Server starts.

View data:
```bash
# Enter MySQL
docker compose exec mysql mysql -u root -photel123 hotel_db

# Query tables
USE hotel_db;
SHOW TABLES;
SELECT * FROM door_event;
```

### 2. Configure M5 Device

#### Upload Firmware to M5CoreS3
1. Open **Arduino IDE**
2. Go `File > Open` and select `m5 and esp32/m5.ino`
3. Set Board: **M5Stack CoreS3**
4. Select correct Port
5. Click **Upload**

#### Setup WiFi & MQTT in M5
Create file `secret.h`:
```cpp
#define ssid "YOUR_WIFI_SSID"
#define password "YOUR_WIFI_PASSWORD"
#define mqtt_server "YOUR_MQTT_SERVER"
#define mqtt_port 1883
#define mqtt_user "mosquitto"
#define mqtt_pass "mosquitto"
```

### 3. Configure ESP32 Node

Upload `m5 and esp32/esp32.ino` to ESP32 using same method

---

## 📱 Usage

### Login Page
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
- Real-time power consumption graph (Watts)
- Access history log showing all door events

#### 2. **Access Control** Tab
- View door status (locked/unlocked)
- Unlock/Lock door instantly from Dashboard
- Event source indicator:
  - 🌐 **WEB** = Command from Dashboard
  - 📺 **M5 Screen** = Command from M5 device
  - 🏷️ **RFID Card** = Card tap authentication

#### 3. **Bookings** Tab
- View all guest room reservations
- Check-in dates and guest information

### Guest Portal
- Guests can enter PIN to access system

---

## 🌟 Features

### ✅ Access Control
- ✓ Unlock/Lock from Dashboard
- ✓ Unlock via RFID card tap
- ✓ Unlock from M5 screen button
- ✓ Auto-lock after 30 seconds

### ✅ Security
- ✓ SHA-256 Hash verification for all Payloads
- ✓ Session-based Authentication
- ✓ Separate Admin and Guest roles

### ✅ Logging & Audit
- ✓ Record all events in Database
- ✓ Export data to CSV (`hotel_data_log.csv`)
- ✓ Correct timestamp (Bangkok Time UTC+7)

### ✅ Real-time Updates
- ✓ Socket.io Push Notifications
- ✓ Live Power Graph
- ✓ Instant Door Status Updates

### ✅ Power Monitoring
- ✓ Measure Current (Amperes) and Power (Watts)
- ✓ Display last 10 readings on graph

---

## 🔍 API Endpoints

### WebSocket Events (Socket.io)

**Client → Server:**
```js
// Request access history
socket.emit('request_history', 'room1');

// Request power history
socket.emit('request_power_history', 'room1');

// Request bookings
socket.emit('request_bookings');

// Send door command
socket.emit('send_control', {
  room: 'room1',
  command: 'open' // or 'close'
});
```

**Server → Client:**
```js
// Real-time door status
socket.on('door_update', (data) => {
  console.log(data.status); // 'open' or 'closed'
  console.log(data.source);  // 'WEB', 'M5 Screen', 'RFID Card'
});

// Real-time power update
socket.on('power_update', (data) => {
  console.log(data.current_amp); // in Amperes
  console.log(data.power_watt);  // in Watts
});
```

### MQTT Topics

```
m5/room1/doorstatus    → Send JSON door status
m5/room1/power         → Send JSON power data
m5/room1/control       → Receive open/close commands
```

---

## 📊 Database Schema

### `door_event` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| room | VARCHAR(20) | Room name (room1, room2) |
| status | VARCHAR(20) | Status (open, closed) |
| source | VARCHAR(50) | Source (WEB, M5 Screen, RFID Card) |
| rfid_uid | VARCHAR(50) | RFID card UID |
| timestamp | TIMESTAMP | Date and time |

### `power_consumption` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| room | VARCHAR(20) | Room name |
| current_amp | FLOAT | Current in Amperes |
| power_watt | FLOAT | Power in Watts |
| timestamp | TIMESTAMP | Date and time |

### `bookings` Table
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Primary Key |
| guest_name | VARCHAR(100) | Guest name |
| room_no | VARCHAR(20) | Room number |
| checkin_date | DATE | Check-in date |
| timestamp | TIMESTAMP | Booking time |

---

## 🐛 Troubleshooting

### ⚠️ Cannot Access Dashboard
```bash
# Check if server is running
docker compose ps

# View logs
docker compose logs hotel_dashboard

# Restart service
docker compose restart hotel_dashboard
```

### ⚠️ MQTT Connection Failed
```bash
# Check MQTT broker
docker compose logs hotel_mqtt

# Test MQTT connection
mosquitto_pub -h localhost -p 1883 -t "test" -m "hello"
mosquitto_sub -h localhost -p 1883 -t "test"
```

### ⚠️ M5 Device Not Connecting
- Verify WiFi SSID/Password in `secret.h`
- Check MQTT server address is correct
- View Serial Monitor (Baud 115200)

### ⚠️ Incorrect Timestamp
Server uses Bangkok Time (UTC+7) automatically, but M5 device may have incorrect system clock. Dashboard uses server time for all events.

### ⚠️ Door Not Auto-closing
- Check delay setting in M5 firmware
- Verify servo motor is calibrated correctly

---

## 👥 Team & Credits

### 📌 Lumina Hotel IoT Dashboard System
**Project Name:** Group Smart Access and Room Management System via IoT Gateway
**Project Type:** Computer Interface Final Project
**Academic Year:** 2025 (Semester 2)
**Institution:** [King Mongkut's University of Technology North Bangkok (KMUTNB)](https://www.kmutnb.ac.th)
**Major:** Robotics Engineering (Year 3)
**Department:** Faculty of Engineering

### 👨‍💻 Team Members

| Name | Student ID | Role |
|------|-----------|------|
| **Thanpisit Banyam** | 6601023611035 | Project Lead / System Architecture |
| **Putthakhun Horthong** | 6601023621022 | Hardware & Firmware Development |
| **Thitaree Siwapornchai** | 6601023620077 | Web Dashboard & Database |

### 🛠️ Responsibilities

**Thanpisit Banyam (6601023611035)**
- Overall project design and architecture
- System integration and testing
- MQTT protocol implementation
- Real-time WebSocket communication
- IoT Gateway management

**Putthakhun Horthong (6601023621022)**
- M5CoreS3 & ESP32 firmware development
- Smart door lock servo motor control
- RFID reader integration & authentication
- Power sensor (ACS712) calibration
- Hardware debugging & optimization

**Thitaree Siwapornchai (6601023620077)**
- Web dashboard UI/UX design
- Backend Node.js server development
- MySQL database schema design
- Authentication & security implementation
- Real-time data monitoring & logging

### 🏆 Acknowledgments

Special thanks to:
- **KMUTNB Faculty of Engineering** for providing resources and guidance
- **Computer Interface Course Instructor** for mentorship and support
- **KMUTNB IoT Laboratory** for equipment and testing facilities
- **All support staff** who contributed to this project

### 📚 Technologies & Tools Used

**Software:**
- Frontend: HTML5, CSS3, JavaScript, Socket.io
- Backend: Node.js, Express.js, MySQL2, PubSubClient
- Database: MySQL 5.7+
- Messaging: MQTT (Eclipse Mosquitto)
- Containerization: Docker, Docker Compose

**Hardware:**
- M5Stack CoreS3 (Main Gateway)
- ESP32 (IoT Node Device)
- RFID Reader Module
- SG90 Servo Motor
- ACS712 Current Sensor
- USB Power Supply

**Development Tools:**
- Arduino IDE
- VS Code
- Git & GitHub
- Docker Desktop
- MySQL Workbench

### 📄 Project Statistics

- **Total Lines of Code:** 2000+
- **Database Tables:** 4 (door_event, power_consumption, bookings, rfid_register)
- **WebSocket Events:** 6 (door_update, power_update, history_data, etc.)
- **MQTT Topics:** 3 (doorstatus, power, control)
- **REST API Endpoints:** 5+ (login, logout, book, etc.)
- **Development Time:** Semester 2, Academic Year 2025
- **Team Size:** 3 Members

### 📞 Contact Information

For questions or inquiries regarding this project:
- 📧 Institution: King Mongkut's University of Technology North Bankok
- 🏫 Department: Faculty of Engineering, Robotics Engineering Program
- 📚 Course: Computer Interface (Final Project)

### 📋 License & Copyright

© 2025 Smart Access and Room Management System via IoT Gateway Team
King Mongkut's University of Technology North Bangkok (KMUTNB)

**Project Members:**
- Thanpisit Banyam (6601023611035)
- Putthakhun Horthong (6601023621022)
- Thitaree Siwapornchai (6601023620077)

**This project is developed as part of the Computer Interface course requirement.**
Students retain the right to use this project for educational and research purposes.
For commercial use or redistribution, please contact KMUTNB administration.

---
**Version:** 1.0.0
**Status:** ✅ Production Ready
