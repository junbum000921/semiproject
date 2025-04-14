#include <Servo.h>
#include <SoftwareSerial.h>

// 서보모터
Servo doorLockServo;
Servo garageServo;

// 블루투스
SoftwareSerial btSerial(6, 7); // RX, TX

// 핀 설정
const int trig1 = 2, echo1 = A5;  // 초음파 센서 1
const int trig2 = 4, echo2 = 5;   // 초음파 센서 2

const int button1 = 8;   // 차고문 열기
const int button2 = 12;  // 차고문 닫기
const int button3 = A3;  // 초인종

const int buzzer = 9;
const int flameSensor = 13;

const int doorLockPin = 10;   // 도어락 서보
const int garageServoPin = 3; // 차고문 서보

const int magSensor = A0;
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

void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);

  pinMode(trig1, OUTPUT); pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT); pinMode(echo2, INPUT);

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);
  pinMode(flameSensor, INPUT);
  pinMode(magSensor, INPUT);
  pinMode(photoSensor, INPUT);

  pinMode(ledGate, OUTPUT);
  pinMode(ledGarage, OUTPUT);

  doorLockServo.attach(doorLockPin);
  garageServo.attach(garageServoPin);

  doorLockServo.write(doorLockPos);
  garageServo.write(garagePos);
}

void loop() {
  handleButtons();
  handleFlame();
  handlePhotoSensor();
  checkUltrasonic(); // 차량 감지
  updateDoorLock();
  updateGarageServo();
  handleBluetooth();
}

void handleButtons() {
  if (digitalRead(button1) == LOW) {
    garageOpening = true;
    garageClosing = false;
  }
  if (digitalRead(button2) == LOW) {
    garageClosing = true;
    garageOpening = false;
  }
  if (digitalRead(button3) == LOW) {
    tone(buzzer, 1000, 200); // 초인종 소리
  }
}

void handleFlame() {
  if (digitalRead(flameSensor) == LOW) {
    tone(buzzer, 2000, 500); // 불꽃 감지
  }
}

void handlePhotoSensor() {
  int light = analogRead(photoSensor);
  if (light < 500) {
    digitalWrite(ledGate, HIGH);  // 어두우면 켬
  } else {
    digitalWrite(ledGate, LOW);   // 밝으면 끔
  }
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

  // 차고문 움직임
  if (now - garagePrevMillis >= garageInterval) {
    garagePrevMillis = now;

    if (garageOpening && garagePos < 120) {
      garagePos++;
      garageServo.write(garagePos);
      if (garagePos == 120) {
        garageOpening = false;
        garageOpenedFlag = true;
        garageOpenedMillis = now;

        digitalWrite(ledGarage, HIGH); // 차고문 열릴 때 불 켜짐
        garageLedOn = true;
        garageLedMillis = now;
      }
    } else if (garageClosing && garagePos > 20) {
      garagePos--;
      garageServo.write(garagePos);
      if (garagePos == 20) {
        garageClosing = false;
        garageOpenedFlag = false;

        digitalWrite(ledGarage, LOW); // 닫히면 꺼짐
        garageLedOn = false;
      }
    }
  }

  // 자동 닫힘 30초 후
  if (garageOpenedFlag && (now - garageOpenedMillis >= 30000)) {
    garageClosing = true;
    garageOpening = false;
    garageOpenedFlag = false;
  }

  // 차고 LED 자동 OFF 30초 후
  if (garageLedOn && (now - garageLedMillis >= 30000)) {
    digitalWrite(ledGarage, LOW);
    garageLedOn = false;
  }
}

void checkUltrasonic() {
  long dist = measureDistance(trig2, echo2);
  if (dist > 0 && dist < 30 && !garageOpening && !garageClosing && garagePos == 20) {
    garageOpening = true;
    garageClosing = false;
  }
}

long measureDistance(int trigPin, int echoPin) {//초음파센서 거리재기
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // 최대 30ms (약 5m)
  long distance = duration * 0.034 / 2;
  return distance;
}-

// 도어락 제어 함수
void openDoorLock() {//대문 열기
  doorLockTarget = 180;
}

void closeDoorLock() {//대문 잠구기
  doorLockTarget = 90;
}

void handleBluetooth() {//대문 블루투스 제어
  if (btSerial.available()) {
    byte cmd = btSerial.read(); // 개행 문자 기준으로 수신

    switch(cmd){
      case 1:
        openDoorLock();
        break;
      case 2:
        closeDoorLock();
        break;
      case 3:
        digitalWrite(ledGarage,HIGH);
        break;
      case 4:
        digitalWrite(ledGarage,LOW);
        break
    }
  }
}
