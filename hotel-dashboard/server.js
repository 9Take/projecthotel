const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// 1. ให้ Node.js เสิร์ฟไฟล์ HTML/CSS จากโฟลเดอร์ public
app.use(express.static('public'));

// 2. ตั้งค่าเชื่อมต่อ MQTT Broker (ใช้ DuckDNS หรือ IP เซิร์ฟเวอร์คุณ)
const mqttClient = mqtt.connect('mqtt://recasa888.duckdns.org:1883', {
    username: 'm5stack', // รหัสที่คุณเพิ่งตั้งไป
    password: '1234'
});

mqttClient.on('connect', () => {
    console.log('🔗 Connected to MQTT Broker!');
    // พอต่อติด ให้ Subscribe รอรับข้อมูลจากทุกห้อง
    mqttClient.subscribe('room/#'); 
});

// 3. เวลามีข้อมูลส่งมาจาก M5Stack ผ่าน MQTT
mqttClient.on('message', (topic, message) => {
    const data = message.toString();
    console.log(`📥 Received: ${topic} -> ${data}`);
    
    // โยนข้อมูลที่ได้ ทะลุไปที่หน้าเว็บ HTML ทันที (ผ่าน Socket.io)
    io.emit('sensor_data', { topic: topic, value: data });
});

// 4. เวลามีคนเปิดหน้าเว็บ และกดปุ่มบนหน้าเว็บ
io.on('connection', (socket) => {
    console.log('💻 User connected to Dashboard');
    
    // รอรับคำสั่งจากปุ่มกดหน้า HTML
    socket.on('send_control', (payload) => {
        // ยิงคำสั่งกลับไปที่ MQTT เพื่อให้ M5Stack รับไปเปิดประตู
        mqttClient.publish('room/door_control', payload);
        console.log(`📤 Command Sent: ${payload}`);
    });
});

// เปิดเซิร์ฟเวอร์ที่พอร์ต 3000
server.listen(3000, () => {
    console.log('🌐 Web Server is running at http://localhost:3000');
});
