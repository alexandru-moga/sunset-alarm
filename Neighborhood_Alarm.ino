#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 53
#define RST_PIN 5
#define PIR_PIN 2
#define BUZZER_PIN 3

MFRC522 mfrc522(SS_PIN, RST_PIN);

bool alarmActive = false;

const char* authorizedUIDs[] = {
  "27B98166",
  "04C8DE2B475980",
  "040F622A7A1690",
  "0493C42B475980"
};
const int authorizedCount = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

String getUIDString(MFRC522::Uid *uid) {
  String uidStr = "";
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(uid->uidByte[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

bool isAuthorized(String uidStr) {
  for (int i = 0; i < authorizedCount; i++) {
    if (uidStr == authorizedUIDs[i]) {
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("System Ready. Waiting for motion...");
}

void loop() {
  if (digitalRead(PIR_PIN) == HIGH && !alarmActive) {
    Serial.println("Motion detected! Alarm activated.");
    alarmActive = true;
    digitalWrite(BUZZER_PIN, HIGH);
  }

  if (alarmActive) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String uidStr = getUIDString(&mfrc522.uid);
      Serial.print("Scanned UID: ");
      Serial.println(uidStr);

      if (isAuthorized(uidStr)) {
        Serial.println("Authorized card! Alarm deactivated.");
        alarmActive = false;
        digitalWrite(BUZZER_PIN, LOW);
        delay(1000);
      } else {
        Serial.println("Unauthorized card.");
      }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}
