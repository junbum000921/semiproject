#include <Servo.h>
#include <SoftwareSerial.h>

// 서보모터
Servo doorLockServo;
Servo garageServo;

// 블루투스
SoftwareSerial btSerial(6, 7); // RX, TX

// 핀 설정
const int trig1 = 2, echo1 = 3; // 초음파 센서 1
const int trig2 = 4, echo2 = 5; // 초음파 센서 2

const int button1 = 8;  // 차고문 버튼1
const int button2 = 12; // 차고문 버튼2
const int button3 = A3; // 초인종

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
  updateDoorLock();
  updateGarageServo();
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
    tone(buzzer, 1000, 200);
  }
}

void handleFlame() {
  if (digitalRead(flameSensor) == LOW) {
    tone(buzzer, 2000, 500);
  }
}

void handlePhotoSensor() {
  int light = analogRead(photoSensor);
  if (light < 500) {
    digitalWrite(ledGate, HIGH);
  } else {
    digitalWrite(ledGate, LOW);
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
  if (now - garagePrevMillis >= garageInterval) {
    garagePrevMillis = now;
    if (garageOpening && garagePos < 120) {
      garagePos++;
      garageServo.write(garagePos);
    } else if (garageClosing && garagePos > 20) {
      garagePos--;
      garageServo.write(garagePos);
    }
  }
}

// 도어락 열고 닫기 함수 (원하는 곳에서 호출)
void openDoorLock() {
  doorLockTarget = 180;
}

void closeDoorLock() {
  doorLockTarget = 90;
}

