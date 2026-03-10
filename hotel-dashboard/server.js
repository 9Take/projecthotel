const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const mysql = require('mysql2');
const session = require('express-session');
const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(session({ secret: 'hotel-secret-key', resave: false, saveUninitialized: true }));
app.use('/assets', express.static('public'));

// ==========================================
// 1. Database Setup (สร้าง 3 ตารางตาม Flowchart)
// ==========================================
const db = mysql.createPool({
    host: 'mysql', user: 'root', password: 'hotel123', database: 'hotel_db',
    waitForConnections: true, connectionLimit: 10, queueLimit: 0
});

const initDB = () => {
    // ตาราง 1: ลงทะเบียนบัตร RFID
    db.query(`CREATE TABLE IF NOT EXISTS rfid_register (
        id INT AUTO_INCREMENT PRIMARY KEY, room VARCHAR(20), rfid_uid VARCHAR(50), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )`);
    // ตาราง 2: ประวัติการเปิด/ปิดประตู
    db.query(`CREATE TABLE IF NOT EXISTS door_event (
        id INT AUTO_INCREMENT PRIMARY KEY, room VARCHAR(20), status VARCHAR(20), source VARCHAR(50), rfid_uid VARCHAR(50), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )`);
    // ตาราง 3: การใช้พลังงาน
    db.query(`CREATE TABLE IF NOT EXISTS power_consumption (
        id INT AUTO_INCREMENT PRIMARY KEY, 
        room VARCHAR(20), 
        current_amp FLOAT, 
        power_watt FLOAT, 
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )`);
    // ตาราง 4: ข้อมูลการจองห้อง
    db.query(`CREATE TABLE IF NOT EXISTS bookings (
        id INT AUTO_INCREMENT PRIMARY KEY, 
        guest_name VARCHAR(100), 
        room_no VARCHAR(20), 
        checkin_date DATE, 
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )`);
    console.log("🗄️ Database Tables (Register, Event, Power) Ready!");
};
initDB();

// ==========================================
// 2. MQTT Setup (รับ JSON)
// ==========================================
const mqttClient = mqtt.connect('mqtt://mqtt:1883', { username: 'm5stack', password: '1234' });

mqttClient.on('connect', () => {
    console.log('🔗 Connected to MQTT Broker!');
    mqttClient.subscribe('m5/+/doorstatus'); // ดักฟังสถานะประตูทุกห้อง
    mqttClient.subscribe('m5/+/power');      // ดักฟังกระแสไฟทุกห้อง
});

mqttClient.on('message', (topic, message) => {
    try {
        const payload = JSON.parse(message.toString()); // แปลงข้อความเป็น JSON Object
        console.log(`📥 Received from [${topic}]:`, payload);

        const parts = topic.split('/');
        const roomNo = parts[1]; // เช่น room1
        const topicType = parts[2]; // เช่น doorstatus หรือ power

        if (topicType === 'doorstatus') {
            // โยนข้อมูลขึ้นหน้าเว็บ
            io.emit('door_update', payload);

            if (payload.Register === true) {
                // ถ้าโหมด Register = true ให้บันทึกเข้าตาราง rfid_register
                if (payload.RFID) {
                    db.query('INSERT INTO rfid_register (room, rfid_uid) VALUES (?, ?)', [roomNo, payload.RFID]);
                    console.log(`💾 [DB] Registered new RFID: ${payload.RFID} for ${roomNo}`);
                }
            } else {
                // ถ้าโหมด Register = false ให้บันทึกเข้าตาราง door_event
                let source = "Unknown";
                if (payload.M5 === true) source = "M5 Screen";
                else if (payload.RFID) source = "RFID Card";

                db.query('INSERT INTO door_event (room, status, source, rfid_uid) VALUES (?, ?, ?, ?)',
                    [roomNo, payload.status, source, payload.RFID]);
                console.log(`💾 [DB] Door Event: ${payload.status} by ${source} at ${roomNo}`);
            }
        }
        else if (topicType === 'power') {
            io.emit('power_update', payload);
            // เปลี่ยนมาใช้ current_amp และ power_watt ตาม JSON ใหม่
            db.query('INSERT INTO power_consumption (room, current_amp, power_watt) VALUES (?, ?, ?)',
                [roomNo, payload.current_amp, payload.power_watt]);
        }
    } catch (error) {
        console.error("❌ Failed to parse JSON or DB Error:", error);
    }
});

// ==========================================
// 3. ระบบ Login & Web Routing
// ==========================================
app.get('/', (req, res) => {
    if (req.session.loggedIn) {
        // 🌟 ถ้าเป็น admin ให้เข้า dashboard ถ้าเป็นคนอื่นให้เข้าหน้า guest
        if (req.session.role === 'admin') {
            res.sendFile(__dirname + '/public/dashboard.html');
        } else {
            res.sendFile(__dirname + '/public/guest.html');
        }
    } else {
        res.sendFile(__dirname + '/public/login.html');
    }
});

app.post('/login', (req, res) => {
    const { username, password } = req.body;
    if ((username === 'admin' && password === '1234') ||
        (username === 'guest1' && password === '1234')) {
        req.session.loggedIn = true;
        req.session.role = username; // เก็บสิทธิ์การใช้งาน
        res.redirect('/');
    } else {
        res.send('<script>alert("Wrong username or password!"); window.location.href="/";</script>');
    }
});

// 🌟 API สำหรับให้หน้า Guest ส่งข้อมูลการจองเข้ามา
app.post('/api/book', (req, res) => {
    const { guest_name, room_no, checkin_date } = req.body;
    db.query('INSERT INTO bookings (guest_name, room_no, checkin_date) VALUES (?, ?, ?)',
        [guest_name, room_no, checkin_date], (err) => {
            if (!err) {
                io.emit('refresh_bookings'); // สั่งให้หน้า Admin รีเฟรชตารางการจอง
                res.send('<script>alert("จองห้องพักสำเร็จ!"); window.location.href="/";</script>');
            } else {
                console.error(err);
                res.send('<script>alert("เกิดข้อผิดพลาด"); window.location.href="/";</script>');
            }
        });
});

// ฟังก์ชัน Logout
app.get('/logout', (req, res) => {
    req.session.destroy(); // ล้างข้อมูลการล็อกอิน (Session)
    res.redirect('/');     // เด้งกลับไปหน้าแรก (หน้า Login)
});

// ==========================================
// 4. WebSocket (รับคำสั่งจาก Dashboard)
// =====================================
io.on('connection', (socket) => {
    console.log('💻 Dashboard Connected');

    // 🌟 ฟังก์ชันใหม่: ดึงประวัติ 50 แถวล่าสุดของห้องนั้นๆ ส่งกลับไปให้หน้าเว็บ
    socket.on('request_history', (room) => {
        db.query('SELECT * FROM door_event WHERE room = ? ORDER BY timestamp DESC LIMIT 50', [room], (err, results) => {
            if (!err) socket.emit('history_data', results);
        });
    });

    // 🌟 Admin ขอข้อมูลการจองทั้งหมด
    socket.on('request_bookings', () => {
        db.query('SELECT * FROM bookings ORDER BY checkin_date ASC', (err, results) => {
            if (!err) socket.emit('booking_data', results);
        });
    });

    // เมื่อกดปุ่มหน้าเว็บ (ทำตาม Flowchart ซ้ายสุด)
    socket.on('send_control', (data) => {
        // 1. สั่งเปิด/ปิด ไปยัง MQTT
        mqttClient.publish(`m5/${data.room}/control`, data.command);
        console.log(`📤 Dashboard Sent Control: ${data.command} to ${data.room}`);

        // 2. บันทึกลง Database ว่าสั่งจาก Dashboard
        db.query('INSERT INTO door_event (room, status, source, rfid_uid) VALUES (?, ?, ?, ?)',
            [data.room, data.command, 'Dashboard', null]);


        // ส่งกลับไปอัปเดตตารางหน้าเว็บให้เห็นด้วย
        io.emit('door_update', {
            Register: false, room: data.room, status: data.command, M5: false, RFID: 'Dashboard (Admin)', Timestamp: new Date().toLocaleTimeString('th-TH')
        });
    });

    // 🌟 ฟังก์ชันใหม่: ดึงประวัติการใช้ไฟ 10 แถวล่าสุดของห้อง เพื่อเอาไปวาดกราฟ
    socket.on('request_power_history', (room) => {
        // ดึง 10 อันดับล่าสุดเรียงตามเวลา (DESC) แล้วส่งกลับไป
        db.query('SELECT current_amp, power_watt FROM power_consumption WHERE room = ? ORDER BY timestamp DESC LIMIT 10', [room], (err, results) => {
            if (!err) {
                // ต้องกลับด้าน Array (Reverse) เพื่อให้กราฟเรียงจากเก่าไปใหม่ (ซ้ายไปขวา)
                socket.emit('power_history_data', { room: room, data: results.reverse() });
            }
        });
    });

});

server.listen(3000, () => console.log('🌐 Server running on port 3000'));