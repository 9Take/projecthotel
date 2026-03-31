# 🔧 QUICK START GUIDE - Installation Instructions

## 🐳 Method 1: Using Docker (Easiest ⭐)

### Step 1: Download Project
```bash
git clone <your-repo-url>
cd projecthotel
```

### Step 2: Configure Environment Variables
```bash
# Copy template
cp .env.example .env

# Edit (optional - defaults work fine)
nano .env
```

**Recommendation:** Change `SECRET_KEY` and passwords for security

### Step 3: Start Docker Services
```bash
# Start all services (first time will download images)
docker compose up -d

# Check status (all should show "Up")
docker compose ps

# View logs
docker compose logs -f
```

### Step 4: Access Dashboard
- Open http://localhost:3000 in browser
- Login:
  - Username: `admin`
  - Password: `admin123`

### ✅ Done! 🎉

---

## 🖥️ Method 2: Manual Installation (Linux/Mac)

### Step 1: Install Prerequisites

#### 1.1 Install Node.js
```bash
# Ubuntu/Debian
curl -sL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Mac
brew install node
```

#### 1.2 Install MySQL
```bash
# Ubuntu/Debian
sudo apt-get install mysql-server

# Mac
brew install mysql@8.0
brew services start mysql@8.0
```

#### 1.3 Install MQTT Broker
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

### Step 3: Setup Node.js Server
```bash
# Navigate to project directory
cd projecthotel/hotel-dashboard

# Install dependencies
npm install

# Copy environment file
cp ../.env.example ../.env
```

### Step 4: Edit .env
```bash
nano ../.env
```

Change these values:
```env
DB_HOST=localhost          # (not 'mysql')
MQTT_HOST=localhost        # (not 'mqtt')
```

### Step 5: Start Server
```bash
# From hotel-dashboard directory
npm start

# Or use nodemon for development
npm install -D nodemon
npx nodemon server.js
```

### Step 6: Access Dashboard
- Open http://localhost:3000
- Login with admin credentials

---

## 📱 Upload Firmware to M5 Device

### Step 1: Download Arduino IDE
- Visit https://www.arduino.cc/en/software
- Install for your OS

### Step 2: Setup M5Stack Board
```
Arduino IDE ➜ File ➜ Preferences

Add this URL:
https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json

File ➜ Boards Manager ➜ Search "M5Stack" ➜ Install
```

### Step 3: Prepare Firmware
```
1. Open Arduino IDE
2. File ➜ Open ➜ m5 and esp32/m5.ino
3. Add required libraries:
   Sketch ➜ Include Library ➜ Manage Libraries
   - M5CoreS3
   - PubSubClient
   - mbedtls
```

### Step 4: Create WiFi Config File
Create `m5 and esp32/secret.h`:
```cpp
#define ssid "YOUR_WIFI_NAME"
#define password "YOUR_WIFI_PASSWORD"
#define mqtt_server "192.168.1.100"  // Your Server IP
#define mqtt_port 1883
#define mqtt_user "mosquitto"
#define mqtt_pass "mosquitto"
```

### Step 5: Upload
```
1. Connect M5 with USB Cable
2. Tools ➜ Select Board ➜ M5Stack CoreS3
3. Tools ➜ Port ➜ /dev/ttyUSB0 (or COM port on Windows)
4. Sketch ➜ Upload
5. Check Serial Monitor (115200 baud) for connection status
```

### Step 6: Test M5 Device
View Serial Monitor output:
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
# View error logs
docker compose logs hotel_dashboard

# Rebuild images
docker compose down
docker compose build --no-cache
docker compose up -d
```

### ❌ Cannot Connect to MQTT
```bash
# Check if MQTT is running
docker compose logs hotel_mqtt

# Test MQTT connection
mosquitto_pub -h localhost -p 1883 -t "test" -m "hello"
```

### ❌ MySQL Connection Error
```bash
# Verify password is correct
# Check DB_HOST in .env

# Try to connect directly
mysql -h localhost -u root -p

# If password lost (Docker):
docker compose exec mysql mysql -u root  # No password needed
```

### ❌ Dashboard Not Loading
```bash
# Wait for server to start (first time takes 30-60 seconds)
docker compose logs -f hotel_dashboard | grep "Server running"

# Refresh browser after "Server running" appears
```

### ❌ M5 Won't Connect to WiFi
```
1. Verify WiFi name/password in secret.h
2. Check Serial Monitor (Baud: 115200)
3. Make sure M5 is connecting to 2.4GHz WiFi (not 5GHz)
```

---

## 🚀 Useful Commands

### Docker Commands
```bash
# Start everything
docker compose up -d

# View logs
docker compose logs -f [service-name]

# Restart a service
docker compose restart hotel_dashboard

# Stop everything
docker compose down

# Remove all data (BE CAREFUL!)
docker compose down -v
```

### Database Commands
```bash
# Enter MySQL
docker compose exec mysql mysql -u root -photel123 hotel_db

# Clear door events
TRUNCATE TABLE door_event;

# View latest events
SELECT * FROM door_event ORDER BY timestamp DESC LIMIT 10;
```

### MQTT Commands
```bash
# Subscribe to topic
docker compose exec mqtt mosquitto_sub -h localhost -t "m5/+/doorstatus"

# Publish test message
docker compose exec mqtt mosquitto_pub -h localhost -t "test/msg" -m "hello"
```

---

## ✅ Verification Checklist

- [ ] Docker Compose starts without errors
- [ ] All 3 containers are running (hotel_dashboard, mysql, mqtt)
- [ ] Can access http://localhost:3000
- [ ] Can login with admin/admin123
- [ ] MySQL has 4 tables
- [ ] M5 Device firmware uploaded successfully
- [ ] Serial Monitor shows "MQTT Connected"
- [ ] Dashboard shows Room 1 & Room 2
- [ ] M5 button press → Dashboard updates realtime

---

## 📞 Still Having Issues?

Check logs with this command:
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

**Please provide:**
- Error message you're seeing
- Output of `docker compose ps`
- Output of `docker compose logs`

---

**🎉 Thanks for using Lumina Hotel IoT Dashboard!**
