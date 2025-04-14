#include <Servo.h>
#include <SoftwareSerial.h>

// 서보모터
Servo doorLockServo;
Servo garageServo;

// 블루투스
SoftwareSerial btSerial(7, 6); // RX, TX
#define BUTTON_PIN 9
// 핀 설정
const int trig1 = 2, echo1 = A5;  // 초음파 센서 1
const int trig2 = 4, echo2 = 5;   // 초음파 센서 2
const int touch = 8;              // 차고문 터치 센서
const int flameSensor = A4;
const int doorLockPin = 10;
const int garageServoPin = 3;
const int magSensor = 13;
const int photoSensor = A1;
const int ledGate = A2;
const int ledGarage = 11;

// 도어락 제어 변수
int doorLockPos = 90;
int doorLockTarget = 90;
unsigned long doorLockPrevMillis = 0;
const long doorLockInterval = 10;

// 차고문 제어 변수
int garagePos = 20;
bool garageOpening = false;
bool garageClosing = false;
unsigned long garagePrevMillis = 0;
const long garageInterval = 40;

// 차고문 자동 닫힘
bool garageOpenedFlag = false;
unsigned long garageOpenedMillis = 0;

// 차고등 LED 제어
unsigned long garageLedMillis = 0;
bool garageLedOn = false;

// 블루투스 전송 버퍼
#define MAX_BUFFER_SIZE 10
byte sendBuffer[MAX_BUFFER_SIZE];
int bufferStart = 0;
int bufferEnd = 0;
unsigned long btSendPrevMillis = 0;
const unsigned long btSendInterval = 2000; // 2초

void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);

  pinMode(trig1, OUTPUT); pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT); pinMode(echo2, INPUT);
  pinMode(touch, INPUT);
  pinMode(flameSensor, INPUT);
  pinMode(magSensor, INPUT);
  pinMode(photoSensor, INPUT);
  pinMode(ledGate, OUTPUT);
  pinMode(ledGarage, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  doorLockServo.attach(doorLockPin);
  garageServo.attach(garageServoPin);

  doorLockServo.write(doorLockPos);
  garageServo.write(garagePos);
}

void loop() {
  handleButtons();
  handleFlame();
  handlePhotoSensor();
  checkUltrasonic();
  updateDoorLock();
  updateGarageServo();
  handleBluetooth();         // 명령 수신
  checkDoorIsOpen();
  handleBluetoothSending();  // 2초 간격 전송
  handleBELL();
}

void handleButtons() {
  if (digitalRead(touch) == LOW) {
    if (!garageOpening && !garageClosing && garagePos == 20) {
      garageOpening = true;
    } else if (!garageOpening && !garageClosing && garagePos == 120) {
      garageClosing = true;
    }
    delay(200);
  }
}
bool lastButtonState = HIGH;  // 처음엔 버튼이 안 눌린 상태라고 가정
void handleBELL(){
  int currentButtonState = digitalRead(BUTTON_PIN);

  // 버튼이 눌렸다가 지금 막 눌린 상태로 바뀐 순간만 감지 (HIGH -> LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    enqueueBT(34);  // 초인종 울리기
  }

  lastButtonState = currentButtonState;  // 상태 업데이트

}

void handleFlame() {
  int value = analogRead(flameSensor);
  if (value < 200) {
    Serial.println("불꽃 감지");
    enqueueBT(90);  // 전송 예약
  }
}

void handlePhotoSensor() {
  int light = analogRead(photoSensor);
  digitalWrite(ledGate, light < 500 ? HIGH : LOW);
}

void updateDoorLock() {
  unsigned long now = millis();
  if (now - doorLockPrevMillis >= doorLockInterval) {
    doorLockPrevMillis = now;
    if (doorLockPos < doorLockTarget) {
      doorLockPos++;
      doorLockServo.write(doorLockPos);
    } else if (doorLockPos > doorLockTarget) {
      doorLockPos--;
      doorLockServo.write(doorLockPos);
    }
  }
}

void updateGarageServo() {
  unsigned long now = millis();

  if (now - garagePrevMillis >= garageInterval) {
    garagePrevMillis = now;

    if (garageOpening && garagePos < 120) {
      garagePos++;
      garageServo.write(garagePos);
      if (garagePos == 120) {
        garageOpening = false;
        garageOpenedFlag = true;
        garageOpenedMillis = now;
        digitalWrite(ledGarage, HIGH);
        garageLedOn = true;
        garageLedMillis = now;
      }
    } else if (garageClosing && garagePos > 20) {
      garagePos--;
      garageServo.write(garagePos);
      if (garagePos == 20) {
        garageClosing = false;
        garageOpenedFlag = false;
        digitalWrite(ledGarage, LOW);
        garageLedOn = false;
      }
    }
  }

  if (garageOpenedFlag && (now - garageOpenedMillis >= 30000)) {
    garageClosing = true;
    garageOpening = false;
    garageOpenedFlag = false;
  }

  if (garageLedOn && (now - garageLedMillis >= 30000)) {
    digitalWrite(ledGarage, LOW);
    garageLedOn = false;
  }
}

void checkUltrasonic() {
  long dist = measureDistance(trig2, echo2);
  if (dist > 0 && dist < 3 && !garageOpening && !garageClosing && garagePos == 20) {
    garageOpening = true;
  }
}

long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void openDoorLock() {
  doorLockTarget = 180;
}

void closeDoorLock() {
  doorLockTarget = 90;
}

void handleBluetooth() {
  if (btSerial.available()) {
    byte cmd = btSerial.read();

    switch (cmd) {
      case 1: openDoorLock(); break;
      case 2: closeDoorLock(); break;
      case 3: digitalWrite(ledGarage, HIGH); break;
      case 4: digitalWrite(ledGarage, LOW); break;
      case 5: garageOpening = true; garageClosing = false; break;
      case 6: garageClosing = true; garageOpening = false; break;
    }
  }
}

int lastDoorState = -1;

void checkDoorIsOpen() {
  int currentState = digitalRead(magSensor);
  if (currentState != lastDoorState) {
    if (currentState == HIGH) {
      Serial.println("문 열림");
      enqueueBT(80);
    } else {
      Serial.println("문 닫힘");
      enqueueBT(81);
    }
    lastDoorState = currentState;
  }
}

// 블루투스 전송 예약 함수
void enqueueBT(byte value) {
  if ((bufferEnd + 1) % MAX_BUFFER_SIZE != bufferStart) {
    sendBuffer[bufferEnd] = value;
    bufferEnd = (bufferEnd + 1) % MAX_BUFFER_SIZE;
  }
}

// 2초마다 하나씩 전송
void handleBluetoothSending() {
  unsigned long now = millis();
  if (now - btSendPrevMillis >= btSendInterval && bufferStart != bufferEnd) {
    btSendPrevMillis = now;

    byte value = sendBuffer[bufferStart];
    btSerial.write(value);
    Serial.println(value);
    bufferStart = (bufferStart + 1) % MAX_BUFFER_SIZE;
  }
}
