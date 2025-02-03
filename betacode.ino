#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ⚡ WiFi Credentials
const char* ssid = "Owxn";            
const char* password = "Owxn2409";    

// ⚡ Telegram Bot Tokens และ Chat IDs
const String botTokens[] = {
    "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q",
    "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8",
    "8175471471:AAG3IpS62xQb_2pR-ZwfZnH_aVMy5ekjukw"
};
const int numBots = sizeof(botTokens) / sizeof(botTokens[0]);

const String chatIds[][2] = { 
    {"-4734652541"}, // บอท 1
    {"-4767274518"},   // บอท 2
    {"6928484464"}    // บอท 3
};
const int chatCounts[] = {2, 2, 2}; // จำนวน chat ต่อบอท

// ⚡ กำหนดขา GPIO สำหรับ IR Sensors (แต่ละลิ้นชักมี 2 ตัว)
const int numDrawers = 5;  // จำนวนลิ้นชัก
const int irSensorPins[numDrawers][2] = { 
    {D1, D2}, {D3, D4}, {D5, D6}, {D7, D8}, {D9, D10} 
}; 

bool previousState[numDrawers][2];  
unsigned long lastTriggerTime[numDrawers][2]; 
const unsigned long debounceTime = 5000;  // 5 วินาที (กำหนดเวลาหน่วง)

// เชื่อมต่อ Telegram Bot
WiFiClientSecure client;
UniversalTelegramBot* bots[numBots];

// ⚡ **WiFi & Telegram Bot Initialization**
void setup() {
    Serial.begin(115200);
    SPI.begin();

    Serial.print("🔗 Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());

    // ⚡ ตั้งค่า Telegram Bots
    client.setInsecure();
    for (int i = 0; i < numBots; i++) {
        bots[i] = new UniversalTelegramBot(botTokens[i], client);
    }

    // ⚡ ตั้งค่า IR Sensors
    for (int i = 0; i < numDrawers; i++) {
        for (int j = 0; j < 2; j++) {
            pinMode(irSensorPins[i][j], INPUT);
            previousState[i][j] = digitalRead(irSensorPins[i][j]);
            lastTriggerTime[i][j] = 0;
        }
    }
}

// ⚡ ฟังก์ชันส่งข้อความไปยังบอทที่ระบุ
void sendNotificationToSpecificBot(int botIndex, const String& message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi ไม่เชื่อมต่อ");
        return;
    }

    Serial.println("📩 Sending notification...");
    for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
        Serial.print("➡ Sending to Chat ID: ");
        Serial.println(chatIds[botIndex][chatIndex]);

        bool sent = bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
        if (sent) {
            Serial.println("✅ Notification sent successfully!");
        } else {
            Serial.println("❌ Failed to send notification. Error: " + String(bots[botIndex]->_lastError));
        }
    }
}

// ⚡ Loop ตรวจสอบเซ็นเซอร์และแจ้งเตือน
void loop() {
    for (int i = 0; i < numDrawers; i++) {
        bool currentState[2] = { digitalRead(irSensorPins[i][0]), digitalRead(irSensorPins[i][1]) };
        unsigned long currentTime = millis();

        // ถ้าเอกสารผ่านเซ็นเซอร์ตัวที่ 1 ไปตัวที่ 2 ถือว่า "เข้า"
        if (previousState[i][0] == HIGH && currentState[0] == LOW) {
            lastTriggerTime[i][0] = currentTime;
        }
        if (previousState[i][1] == HIGH && currentState[1] == LOW) {
            if (currentTime - lastTriggerTime[i][0] < debounceTime) {
                Serial.println("📥 Document ENTERED in drawer " + String(i + 1));
                sendNotificationToSpecificBot(0, "📥 มีเอกสารถูกใส่ในลิ้นชักที่ " + String(i + 1));
            }
        }

        // ถ้าเอกสารผ่านเซ็นเซอร์ตัวที่ 2 ไปตัวที่ 1 ถือว่า "ออก"
        if (previousState[i][1] == HIGH && currentState[1] == LOW) {
            lastTriggerTime[i][1] = currentTime;
        }
        if (previousState[i][0] == HIGH && currentState[0] == LOW) {
            if (currentTime - lastTriggerTime[i][1] < debounceTime) {
                Serial.println("📤 Document EXITED from drawer " + String(i + 1));
                sendNotificationToSpecificBot(0, "📤 มีเอกสารถูกนำออกจากลิ้นชักที่ " + String(i + 1));
            }
        }

        // อัปเดตสถานะล่าสุดของเซ็นเซอร์
        previousState[i][0] = currentState[0];
        previousState[i][1] = currentState[1];
    }

    delay(500); // ลดการทำงานของ Loop
}
