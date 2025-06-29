#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* apiUrl = "http://192.168.1.144/alarm";

#define PIR_PIN 22

#define US1_TRIG 17
#define US1_ECHO 16
#define US2_TRIG 4
#define US2_ECHO 15

#define IR_PIN 21

#define BUZZER_PIN 2

#define RGB_RED 25
#define RGB_GREEN 26
#define RGB_BLUE 27

#define RFID_SS_PIN 5
#define RFID_RST_PIN 0

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

IRrecv irrecv(IR_PIN, 1024, 15, true);
decode_results results;

bool alarmTriggered = false;
unsigned long lastTriggerTime = 0;

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_RED, r);
  analogWrite(RGB_GREEN, g);
  analogWrite(RGB_BLUE, b);
}

void triggerAlarm(const char* cause) {
  if (!alarmTriggered) {
    alarmTriggered = true;
    lastTriggerTime = millis();
    digitalWrite(BUZZER_PIN, HIGH);
    setRGB(255, 0, 0);
    Serial.printf("ALARM TRIGGERED by %s\n", cause);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(apiUrl);
      http.addHeader("Content-Type", "application/json");
      String payload = String("{\"cause\":\"") + cause + "\"}";
      int httpCode = http.POST(payload);
      Serial.printf("API POST code: %d\n", httpCode);
      http.end();
    }
  }
}

float readUltrasonic(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(US1_TRIG, OUTPUT); pinMode(US1_ECHO, INPUT);
  pinMode(US2_TRIG, OUTPUT); pinMode(US2_ECHO, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  pinMode(RGB_RED, OUTPUT); pinMode(RGB_GREEN, OUTPUT); pinMode(RGB_BLUE, OUTPUT);

  setRGB(0, 0, 255); 

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected");

  SPI.begin();
  rfid.PCD_Init();

  irrecv.enableIRIn();

  setRGB(0, 255, 0);
}

void loop() {
  if (digitalRead(PIR_PIN) == HIGH) {
    triggerAlarm("PIR");
  }

  float us1 = readUltrasonic(US1_TRIG, US1_ECHO);
  float us2 = readUltrasonic(US2_TRIG, US2_ECHO);
  if ((us1 > 0 && us1 < 50) || (us2 > 0 && us2 < 50)) {
    triggerAlarm("Ultrasonic");
  }

  if (irrecv.decode(&results)) {
    triggerAlarm("IR");
    Serial.printf("IR code: 0x%X\n", results.value);
    irrecv.resume();
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("RFID UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) Serial.printf("%02X ", rfid.uid.uidByte[i]);
    Serial.println();
    triggerAlarm("RFID");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  if (alarmTriggered && millis() - lastTriggerTime > 10000) {
    alarmTriggered = false;
    digitalWrite(BUZZER_PIN, LOW);
    setRGB(0, 255, 0);
    Serial.println("Alarm reset");
  }

  delay(100);
}
