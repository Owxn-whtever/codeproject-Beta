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

// ⚡ กำหนดขา GPIO สำหรับ IR Sensor
const int irSensorPins[] = {D1, D2, D3, D4, D5}; 
const int numSensors = sizeof(irSensorPins) / sizeof(irSensorPins[0]);

bool previousState[numSensors]; 
unsigned long lastDocumentTime[numSensors]; 
const unsigned long debounceTime = 5000;  // เปลี่ยนเวลาหน่วงเป็น 5 วินาที

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
    client.setInsecure(); // ปิดการตรวจสอบ SSL
    for (int i = 0; i < numBots; i++) {
        bots[i] = new UniversalTelegramBot(botTokens[i], client);
    }

    // ⚡ ตั้งค่า IR Sensors
    for (int i = 0; i < numSensors; i++) {
        pinMode(irSensorPins[i], INPUT);
        previousState[i] = digitalRead(irSensorPins[i]);
        lastDocumentTime[i] = 0;
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
            // ใช้ _lastError แทน getError
            Serial.println("❌ Failed to send notification. Error: " + String(bots[botIndex]->_lastError));
        }
    }
}

// ⚡ Loop ตรวจสอบเซ็นเซอร์และแจ้งเตือน
void loop() {
    for (int i = 0; i < numSensors; i++) {
        bool currentState = digitalRead(irSensorPins[i]);

        // ถ้าสถานะเปลี่ยนจาก HIGH -> LOW (มีการใส่เอกสาร)
        if (previousState[i] == HIGH && currentState == LOW) {
            unsigned long currentTime = millis();
            if (currentTime - lastDocumentTime[i] > debounceTime) {
                Serial.println("📄 Document detected in drawer " + String(i + 1));
                sendNotificationToSpecificBot(0, "📄 มีเอกสารถูกใส่ในลิ้นชักที่ " + String(i + 1));
                lastDocumentTime[i] = currentTime;
            }
        }
        previousState[i] = currentState;
    }

    delay(1000); // ลดการทำงานของ Loop
}
