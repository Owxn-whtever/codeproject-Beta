#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// **WiFi Credentials**
const char* ssid = "Owxn";
const char* password = "Owxn2409";

// **Telegram Bot Token และ Chat IDs**
const char* botTokens[] = {
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot 1
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q"  // Bot 2
};
const int numBots = sizeof(botTokens) / sizeof(botTokens[0]); // จำนวน Bot

// **Chat IDs ของแต่ละ Bot**
const char* chatIdsBot1[] = {"6928484464"}; // Chat ID ของ Bot 1
const char* chatIdsBot2[] = {"6928484464"}; // Chat ID ของ Bot 2
const char** chatIds[] = {chatIdsBot1, chatIdsBot2}; // รวมทุก Chat ID
const int chatCounts[] = {sizeof(chatIdsBot1) / sizeof(chatIdsBot1[0]),
                          sizeof(chatIdsBot2) / sizeof(chatIdsBot2[0])}; // จำนวน Chat ID ของแต่ละ Bot

// **สร้าง Client สำหรับ WiFi และ Telegram Bot**
WiFiClientSecure client;
UniversalTelegramBot* bots[numBots];

// **พินสำหรับเซ็นเซอร์**
const int sensorPin1 = D1; // IR Sensor 1 (Digital Pin)
const int sensorPin2 = D2; // IR Sensor 2 (Digital Pin)
bool drawerOccupied1 = false;   // สถานะเซ็นเซอร์ 1
bool drawerOccupied2 = false;   // สถานะเซ็นเซอร์ 2

bool wifiConnected = false; // ติดตามสถานะ Wi-Fi

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin1, INPUT); // ตั้งเซ็นเซอร์ 1 เป็น Input
  pinMode(sensorPin2, INPUT); // ตั้งเซ็นเซอร์ 2 เป็น Input

  // **เชื่อมต่อ WiFi**
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  wifiConnected = true; // อัปเดตสถานะ Wi-Fi เป็นเชื่อมต่อแล้ว
  client.setInsecure(); // ข้ามการตรวจสอบ SSL Certificate

  // **สร้าง Bot สำหรับแต่ละ Token**
  for (int i = 0; i < numBots; i++) {
    bots[i] = new UniversalTelegramBot(botTokens[i], client);
  }

  // แจ้งเตือนการเชื่อมต่อ Wi-Fi
  sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi เรียบร้อยแล้ว!");
}

void sendNotificationToAll(const String& message) {
  for (int botIndex = 0; botIndex < numBots; botIndex++) {
    for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
      bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
      Serial.println("Notification sent to Bot " + String(botIndex + 1) + 
                     " (Chat ID: " + String(chatIds[botIndex][chatIndex]) + ")");
    }
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    Serial.println("Wi-Fi disconnected!");
    sendNotificationToAll("การเชื่อมต่อ Wi-Fi หลุด! โปรดตรวจสอบเครือข่าย.");
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    Serial.println("Wi-Fi reconnected!");
    sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi ได้อีกครั้ง!");
  }
}

void checkDrawerSensor(int sensorPin, bool &drawerOccupied, const String &drawerName) {
  int sensorValue = digitalRead(sensorPin);
  if ((sensorValue == LOW) && !drawerOccupied) { // LOW หมายถึงตรวจจับเอกสาร
    sendNotificationToAll("เอกสารถูกวางในลิ้นชักแล้ว");
    drawerOccupied = true; // อัปเดตสถานะเซ็นเซอร์
  } else if ((sensorValue == HIGH) && drawerOccupied) { // HIGH หมายถึงไม่มีเอกสาร
    Serial.println(drawerName + " is empty.");
    drawerOccupied = false;
  }
}

void loop() {
  // ตรวจสอบการเชื่อมต่อ Wi-Fi
  checkWiFiConnection();

  // ตรวจสอบสถานะของแต่ละเซ็นเซอร์
  checkDrawerSensor(sensorPin1, drawerOccupied1, "ลิ้นชักที่ 1");
  checkDrawerSensor(sensorPin2, drawerOccupied2, "ลิ้นชักที่ 2");

  delay(100); // ลดความถี่ในการอ่านค่าเพื่อป้องกันการ Overload
}

