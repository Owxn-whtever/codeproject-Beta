#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ✅ WiFi Credentials
const char* ssid = "Owxn";            
const char* password = "Owxn2409";    

// ✅ Telegram Bot Tokens (1 บอทต่อ 1 ชั้น)
const char* botTokens[] = {
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q", // Bot ชั้น 1
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot ชั้น 2
  "8175471471:AAG3IpS62xQb_2pR-ZwfZnH_aVMy5ekjukw", // Bot ชั้น 3
  "", // Bot ชั้น 4 (ยังไม่มี)
  ""  // Bot ชั้น 5 (ยังไม่มี)
};

// ✅ Chat IDs ของแต่ละบอท (1 ชั้นต่อ 1 บอท)
const char* chatIds[] = {
  "-4734652541",  // Chat ID ชั้น 1
  "-4767274518",  // Chat ID ชั้น 2
  "6928484464",   // Chat ID ชั้น 3
  "",             // Chat ID ชั้น 4 (ยังไม่มี)
  ""              // Chat ID ชั้น 5 (ยังไม่มี)
};

// ✅ จำนวนชั้นที่ใช้งาน
const int numDrawers = sizeof(botTokens) / sizeof(botTokens[0]);

// ✅ กำหนด GPIO สำหรับ IR Sensors (1 คู่ต่อ 1 ชั้น)
const int irPins[][2] = {
    {D0, D1},  // ชั้น 1
    {D2, D3},  // ชั้น 2
    {D4, D5},  // ชั้น 3
    {D6, D7},  // ชั้น 4
    {D8, D9}   // ชั้น 5 (แก้ RX/TX เป็นขาอื่น)
};

// ✅ ตัวแปรสถานะ
bool previousState[numDrawers][2]; 
unsigned long lastTriggerTime[numDrawers]; 
const unsigned long debounceTime = 3000; // ป้องกันการตรวจจับซ้ำ (3 วินาที)

// ✅ WiFiClientSecure และ UniversalTelegramBot
WiFiClientSecure client;
UniversalTelegramBot* bots[numDrawers]; // 1 บอทต่อ 1 ชั้น

// ✅ Setup Function
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

    // ⚡ ปิดการตรวจสอบ SSL
    client.setInsecure();

    // ⚡ ตั้งค่า IR Sensors
    for (int i = 0; i < numDrawers; i++) {
        pinMode(irPins[i][0], INPUT);
        pinMode(irPins[i][1], INPUT);
        previousState[i][0] = digitalRead(irPins[i][0]);
        previousState[i][1] = digitalRead(irPins[i][1]);
        lastTriggerTime[i] = 0;
    }

    // ✅ สร้างบอทแต่ละตัว (1 ชั้นต่อ 1 บอท)
    for (int i = 0; i < numDrawers; i++) {
        if (strlen(botTokens[i]) > 0) {
            bots[i] = new UniversalTelegramBot(botTokens[i], client);
        } else {
            bots[i] = nullptr;  // ถ้าไม่มีโทเคน ให้กำหนดเป็น nullptr
        }
    }
}

// ✅ ฟังก์ชันส่งข้อความไปยัง Telegram (เฉพาะบอทของชั้นนั้น)
void sendNotification(int drawer, String direction) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi ไม่เชื่อมต่อ");
        return;
    }

    if (bots[drawer] == nullptr || strlen(chatIds[drawer]) == 0) {
        Serial.println("⚠️ ไม่มีบอทสำหรับชั้นนี้ หรือไม่มี Chat ID");
        return;
    }

    String message = "📄 เอกสาร " + direction + " ที่ลิ้นชัก " + String(drawer + 1);
    Serial.println("📩 " + message);

    bool sent = bots[drawer]->sendMessage(chatIds[drawer], message, "Markdown");
    if (sent) {
        Serial.println("✅ ส่งข้อความสำเร็จไปยัง Bot ชั้น " + String(drawer + 1));
    } else {
        Serial.println("❌ ส่งข้อความล้มเหลวที่ Bot ชั้น " + String(drawer + 1));
    }
}

// ✅ Loop ตรวจสอบเซ็นเซอร์
void loop() {
    for (int i = 0; i < numDrawers; i++) {
        bool currentState1 = digitalRead(irPins[i][0]);
        bool currentState2 = digitalRead(irPins[i][1]);

        unsigned long currentTime = millis();

        // ตรวจจับเอกสาร "เข้า" (Sensor 1 -> Sensor 2)
        if (previousState[i][0] == LOW && currentState1 == HIGH && 
            previousState[i][1] == HIGH && currentState2 == LOW) {
            if (currentTime - lastTriggerTime[i] > debounceTime) {
                sendNotification(i, "เข้า");
                lastTriggerTime[i] = currentTime;
            }
        }

        // ตรวจจับเอกสาร "ออก" (Sensor 2 -> Sensor 1)
        if (previousState[i][1] == LOW && currentState2 == HIGH && 
            previousState[i][0] == HIGH && currentState1 == LOW) {
            if (currentTime - lastTriggerTime[i] > debounceTime) {
                sendNotification(i, "ออก");
                lastTriggerTime[i] = currentTime;
            }
        }

        previousState[i][0] = currentState1;
        previousState[i][1] = currentState2;
    }

    delay(100);
}
