#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// **WiFi Credentials**
const char* ssid = "Owxn"; // SSID ของ Wi-Fi
const char* password = "Owxn2409"; // รหัสผ่านของ Wi-Fi

// **Telegram Bot Token และ Chat IDs**
const char* botTokens[] = { // โทเค็นของ Telegram Bots
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot 1
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q", // Bot 2
  "7731694722:AAGIyRqH4XgT-Bh48aQWDWks0IN9x7mzveo", // Bot 3
  "Token bot", // Bot 4
  "Token bot"  // Bot 5
};

const int numBots = sizeof(botTokens) / sizeof(botTokens[0]); // จำนวนบอททั้งหมด
const char* chatIdsBot1[] = {"-4767274518"}; // Chat ID ของ Bot 1
const char* chatIdsBot2[] = {"-4734652541"}; // Chat ID ของ Bot 2
const char* chatIdsBot3[] = {"-4637803081"}; // Chat ID ของ Bot 3
const char* chatIdsBot4[] = {""}; // Chat ID ของ Bot 4
const char* chatIdsBot5[] = {"Chai id"}; // Chat ID ของ Bot 5

// เก็บ Chat IDs และจำนวน Chat IDs สำหรับแต่ละบอท
const char** chatIds[] = {chatIdsBot1, chatIdsBot2, chatIdsBot3, chatIdsBot4, chatIdsBot5};
const int chatCounts[] = {
  sizeof(chatIdsBot1) / sizeof(chatIdsBot1[0]),
  sizeof(chatIdsBot2) / sizeof(chatIdsBot2[0]),
  sizeof(chatIdsBot3) / sizeof(chatIdsBot3[0]),
  sizeof(chatIdsBot4) / sizeof(chatIdsBot4[0]),
  sizeof(chatIdsBot5) / sizeof(chatIdsBot5[0])
};

WiFiClientSecure client; // ใช้สำหรับการเชื่อมต่อ HTTPS
UniversalTelegramBot* bots[numBots]; // อาร์เรย์สำหรับเก็บบอท

// **พินสำหรับเซ็นเซอร์**
const int sensorPins[] = {D1, D2, D3, D4, D5}; // พินของเซ็นเซอร์แต่ละลิ้นชัก
bool drawerOccupied[] = {false, false, false, false, false}; // สถานะของลิ้นชักแต่ละตัว (ว่างหรือมีเอกสาร)

// **Array สำหรับเก็บเอกสารที่ซ้อนกัน**
String documentStack[numBots][10]; // แต่ละลิ้นชักสามารถเก็บเอกสารได้สูงสุด 10 ชุด
int documentCounts[] = {0, 0, 0, 0, 0}; // จำนวนเอกสารในแต่ละลิ้นชัก

void setup() {
  Serial.begin(115200); // เริ่มต้น Serial Monitor
  for (int i = 0; i < numBots; i++) {
    pinMode(sensorPins[i], INPUT); // ตั้งค่าพินเซ็นเซอร์เป็น Input
  }

  // เชื่อมต่อ Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // รอจนกว่า Wi-Fi จะเชื่อมต่อ
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  client.setInsecure(); // ปิดการตรวจสอบใบรับรอง SSL/TLS

  for (int i = 0; i < numBots; i++) {
    bots[i] = new UniversalTelegramBot(botTokens[i], client); // สร้าง UniversalTelegramBot สำหรับแต่ละ Token
  }

  // แจ้งเตือนเมื่อเชื่อมต่อ Wi-Fi เสร็จสิ้น
  sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi เรียบร้อยแล้ว!");
}

// ฟังก์ชันส่งข้อความแจ้งเตือนถึงทุกบอท
void sendNotificationToAll(const String& message) {
  for (int botIndex = 0; botIndex < numBots; botIndex++) {
    for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
      bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
    }
  }
}

void manageDocumentStack(int botIndex, const String& documentID) {
  // ตรวจสอบว่ามีเอกสารอยู่ในลิ้นชักแล้วหรือไม่
  bool exists = false;
  for (int i = 0; i < documentCounts[botIndex]; i++) {
    if (documentStack[botIndex][i] == documentID) {
      exists = true; // เอกสารนี้มีอยู่แล้ว
      break;
    }
  }

  // ถ้าไม่มีเอกสาร ให้เพิ่มเข้าไป
  if (!exists) {
    if (documentCounts[botIndex] < 10) { // ตรวจสอบว่าไม่เกินความจุสูงสุด
      documentStack[botIndex][documentCounts[botIndex]++] = documentID;
      sendNotificationToSpecificBot(botIndex, 
        "มีเอกสารใหม่เข้าในลิ้นชัก! เอกสาร ID: " + documentID +
        " (จำนวนเอกสาร: " + String(documentCounts[botIndex]) + ")");
    } else {
      sendNotificationToSpecificBot(botIndex, 
        "ลิ้นชักเต็มแล้ว! ไม่สามารถเพิ่มเอกสารใหม่ (ID: " + documentID + ")");
    }
  }
}

// ฟังก์ชันส่งข้อความแจ้งเตือนถึงบอทที่ระบุ
void sendNotificationToSpecificBot(int botIndex, const String& message) {
  for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
    bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
  }
}

// ฟังก์ชันตรวจสอบสถานะเซ็นเซอร์แต่ละลิ้นชัก (ปรับปรุงเล็กน้อย)
void checkDrawerSensor(int sensorPin, bool& occupied, int botIndex) {
  int sensorValue = digitalRead(sensorPin); // อ่านค่าจากเซ็นเซอร์
  String documentID = "DOC-" + String(sensorPin); // สร้าง ID สำหรับเอกสาร

  // ตรวจจับเมื่อมีเอกสารใหม่เข้ามา
  if (sensorValue == LOW && !occupied) {
    manageDocumentStack(botIndex, documentID); // เพิ่มเอกสารใหม่
    occupied = true; // อัปเดตสถานะลิ้นชัก
  } 
  // ตรวจจับเมื่อเอกสารถูกนำออกไป
  else if (sensorValue == HIGH && occupied) {
    sendNotificationToSpecificBot(botIndex, 
      "ลิ้นชักว่างแล้ว! (เอกสารที่เหลือ: " + String(documentCounts[botIndex]) + ")");
    occupied = false; // อัปเดตสถานะลิ้นชัก
  }
}

void loop() {
  for (int i = 0; i < numBots; i++) { // ตรวจสอบสถานะของเซ็นเซอร์แต่ละลิ้นชัก
    checkDrawerSensor(sensorPins[i], drawerOccupied[i], i);
  }
  delay(100); // ลดความถี่ในการตรวจสอบเพื่อประหยัดพลังงาน
}
