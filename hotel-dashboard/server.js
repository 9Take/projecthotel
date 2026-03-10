const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const mysql = require('mysql2');
const session = require('express-session');
const fs = require('fs'); // 🌟 เพิ่ม: สำหรับเขียนไฟล์ CSV
const crypto = require('crypto'); // 🌟 เพิ่ม: สำหรับทำ Checksum/Hash

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(session({ secret: process.env.SESSION_SECRET, resave: false, saveUninitialized: true }));
app.use('/assets', express.static('public'));

// ==========================================
// 🌟 ตั้งค่ารหัสลับสำหรับ Payload Hash 🌟
// ==========================================
const SECRET_KEY = process.env.SECRET_KEY;

// ==========================================
// 1. Database Setup 
// ==========================================
const db = mysql.createPool({
    host: process.env.DB_HOST,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME,
    waitForConnections: true, connectionLimit: 10, queueLimit: 0
});

const initDB = () => {
    db.query(`CREATE TABLE IF NOT EXISTS rfid_register (id INT AUTO_INCREMENT PRIMARY KEY, room VARCHAR(20), rfid_uid VARCHAR(50), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)`);
    db.query(`CREATE TABLE IF NOT EXISTS door_event (id INT AUTO_INCREMENT PRIMARY KEY, room VARCHAR(20), status VARCHAR(20), source VARCHAR(50), rfid_uid VARCHAR(50), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)`);
    db.query(`CREATE TABLE IF NOT EXISTS power_consumption (id INT AUTO_INCREMENT PRIMARY KEY, room VARCHAR(20), current_amp FLOAT, power_watt FLOAT, timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)`);
    db.query(`CREATE TABLE IF NOT EXISTS bookings (id INT AUTO_INCREMENT PRIMARY KEY, guest_name VARCHAR(100), room_no VARCHAR(20), checkin_date DATE, timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)`);
    console.log("🗄️ Database Tables Ready!");

    // 🌟 สร้างไฟล์ CSV ทิ้งไว้ถ้ายังไม่มี
    if (!fs.existsSync('hotel_data_log.csv')) {
        fs.writeFileSync('hotel_data_log.csv', 'Timestamp,Topic,Room,Data_Payload\n');
        console.log("📄 CSV Log File Created!");
    }
};
initDB();

// ==========================================
// 2. MQTT Setup (รับ JSON)
// ==========================================
const mqttClient = mqtt.connect(process.env.MQTT_HOST, {
    username: process.env.MQTT_USER,
    password: process.env.MQTT_PASSWORD
});
mqttClient.on('connect', () => {
    console.log('🔗 Connected to MQTT Broker!');
    mqttClient.subscribe('m5/+/doorstatus');
    mqttClient.subscribe('m5/+/power');
});

mqttClient.on('message', (topic, message) => {
    try {
        const payload = JSON.parse(message.toString());
        const parts = topic.split('/');
        const roomNo = parts[1];
        const topicType = parts[2];

        // ========================================================
        // 🌟 FEATURE 1: PAYLOAD CHECKSUM / HASH VERIFICATION 🌟
        // ========================================================
        // สูตรคือ: เอาชื่อห้อง มารวมกับ SECRET_KEY แล้วเข้ารหัส SHA-256
        const expectedHash = crypto.createHash('sha256').update(roomNo + SECRET_KEY).digest('hex');

        if (payload.hash !== expectedHash) {
            console.error(`🚨 SECURITY ALERT: ข้อมูลถูกปลอมแปลง หรือ Hash ไม่ตรงกัน! (Room: ${roomNo})`);
            console.error(`   -> Expected: ${expectedHash}`);
            console.error(`   -> Received: ${payload.hash}`);
            return; // ⛔ เตะทิ้งทันที ไม่บันทึกลงฐานข้อมูล
        }
        console.log(`✅ Hash Verified for ${roomNo}`);

        // ========================================================
        // 🌟 FEATURE 2: DATA LOGGING - TEXT-BASED (CSV) 🌟
        // ========================================================
        const timeNow = new Date().toLocaleString('th-TH');
        const cleanPayload = JSON.stringify(payload).replace(/,/g, ';'); // เปลี่ยนลูกน้ำใน JSON เป็น ; จะได้ไม่กวน CSV
        const csvLine = `"${timeNow}","${topic}","${roomNo}","${cleanPayload}"\n`;

        fs.appendFile('hotel_data_log.csv', csvLine, (err) => {
            if (err) console.error("❌ CSV Write Error:", err);
        });

        // --- ทำงานปกติ (บันทึกลง Database & โชว์หน้าเว็บ) ---
        if (topicType === 'doorstatus') {
            io.emit('door_update', payload);
            if (payload.Register === true && payload.RFID) {
                db.query('INSERT INTO rfid_register (room, rfid_uid) VALUES (?, ?)', [roomNo, payload.RFID]);
            } else {
                let source = "Unknown";
                if (payload.M5 === true) source = "M5 Screen";
                else if (payload.RFID) source = "RFID Card";
                db.query('INSERT INTO door_event (room, status, source, rfid_uid) VALUES (?, ?, ?, ?)', [roomNo, payload.status, source, payload.RFID]);
            }
        }
        else if (topicType === 'power') {
            io.emit('power_update', payload);
            db.query('INSERT INTO power_consumption (room, current_amp, power_watt) VALUES (?, ?, ?)', [roomNo, payload.current_amp, payload.power_watt]);
        }
    } catch (error) {
        console.error("❌ Failed to parse JSON:", error);
    }
});

// ==========================================
// 3. ระบบ Login & Web Routing
// ==========================================
app.get('/', (req, res) => req.session.loggedIn ? (req.session.role === 'admin' ? res.sendFile(__dirname + '/public/dashboard.html') : res.sendFile(__dirname + '/public/guest.html')) : res.sendFile(__dirname + '/public/login.html'));
app.post('/login', (req, res) => {
    const { username, password } = req.body;

    // 🌟 ดึง User/Pass จาก .env มาเทียบ
    const isAdmin = username === process.env.ADMIN_USER && password === process.env.ADMIN_PASS;
    const isGuest = username === process.env.GUEST_USER && password === process.env.GUEST_PASS;

    if (isAdmin || isGuest) {
        req.session.loggedIn = true;
        req.session.role = username;
        res.redirect('/');
    } else {
        res.send('<script>alert("Wrong password!"); window.location.href="/";</script>');
    }
});

app.post('/api/book', (req, res) => {
    db.query('INSERT INTO bookings (guest_name, room_no, checkin_date) VALUES (?, ?, ?)', [req.body.guest_name, req.body.room_no, req.body.checkin_date], (err) => {
        if (!err) { io.emit('refresh_bookings'); res.send('<script>alert("จองสำเร็จ!"); window.location.href="/";</script>'); }
        else res.send('<script>alert("Error"); window.location.href="/";</script>');
    });
});
app.get('/logout', (req, res) => { req.session.destroy(); res.redirect('/'); });

// ==========================================
// 4. WebSocket (รับคำสั่งจาก Dashboard)
// ==========================================
io.on('connection', (socket) => {
    socket.on('request_history', (room) => db.query('SELECT * FROM door_event WHERE room = ? ORDER BY timestamp DESC LIMIT 50', [room], (err, res) => { if (!err) socket.emit('history_data', res); }));
    socket.on('request_bookings', () => db.query('SELECT * FROM bookings ORDER BY checkin_date ASC', (err, res) => { if (!err) socket.emit('booking_data', res); }));
    socket.on('request_power_history', (room) => db.query('SELECT current_amp, power_watt FROM power_consumption WHERE room = ? ORDER BY timestamp DESC LIMIT 10', [room], (err, res) => { if (!err) socket.emit('power_history_data', { room: room, data: res.reverse() }); }));

    socket.on('send_control', (data) => {
        mqttClient.publish(`m5/${data.room}/control`, data.command);
        db.query('INSERT INTO door_event (room, status, source, rfid_uid) VALUES (?, ?, ?, ?)', [data.room, data.command, 'Dashboard', null]);
        io.emit('door_update', { Register: false, room: data.room, status: data.command, M5: false, RFID: 'Dashboard (Admin)', Timestamp: new Date().toLocaleTimeString('th-TH') });
    });
});

server.listen(3000, () => console.log('🌐 Server running on port 3000'));