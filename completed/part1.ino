#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>

// 서보모터
Servo doorLockServo;
Servo garageServo;

// 블루투스
SoftwareSerial btSerial(7,6); // RX, TX
#define BUTTON_PIN 9

// 핀 설정
const int trig1 = 12, echo1 = 2;
const int trig2 = 4, echo2 = 5;
const int touch = 8;
const int flameSensor = A0;
const int doorLockPin = 10;
const int garageServoPin = 3;
const int magSensor = 13;
const int photoSensor = A1;
const int ledGate = A2;
const int ledGarage = 11;

// 도어락
int doorLockPos = 90;
int doorLockTarget = 90;
unsigned long doorLockPrevMillis = 0;
const long doorLockInterval = 10;

// 차고문
int garagePos = 0;
bool garageOpening = false;
bool garageClosing = false;
unsigned long garagePrevMillis = 0;
const long garageInterval = 15;
const int garageStep = 4;

// 자동 닫힘 및 LED
bool garageOpenedFlag = false;
unsigned long garageOpenedMillis = 0;
unsigned long garageLedMillis = 0;
bool garageLedOn = false;

// 초음파 재감지 방지
unsigned long recentGarageActionMillis = 0;
const unsigned long garageCooldown = 5000;

// 블루투스 버퍼
#define MAX_BUFFER_SIZE 10
byte sendBuffer[MAX_BUFFER_SIZE];
int bufferStart = 0;
int bufferEnd = 0;
unsigned long btSendPrevMillis = 0;
const unsigned long btSendInterval = 2000;
#define SLAVE_ADDRESS 2  // 슬레이브 주소
void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);
  Wire.begin();

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
  delay(1000);  // 초기 대기
}

void loop() {
  handleButtons();
  handleFlame();
  handlePhotoSensor();
  checkUltrasonic();
  checkUltrasonic2();
  updateDoorLock();
  updateGarageServo();
  handleBluetooth();
  checkDoorIsOpen();
  handleBluetoothSending();
  handleBELL();
  i2cwire();
}

void handleButtons() {
  if (digitalRead(touch) == HIGH) {
    if (!garageOpening && !garageClosing && garagePos <= 5) {
      garageOpening = true;
      recentGarageActionMillis = millis();
    } else if (!garageOpening && !garageClosing && garagePos >= 115) {
      garageClosing = true;
      recentGarageActionMillis = millis();
    }
    delay(200);
  }
}
unsigned long i2cRequestPrevMillis = 0;
const unsigned long i2cRequestInterval = 5000;  // 3초마다 요청

void i2cwire() {
  unsigned long now = millis();
  if (now - i2cRequestPrevMillis >= i2cRequestInterval) { // 5초마다 요청
    Wire.requestFrom(SLAVE_ADDRESS, 3);  // 3바이트 요청 (온도, 습도, 미세먼지)

    if (Wire.available() >= 3) {
      int t = Wire.read();  // 온도 값 수신
      int h = Wire.read();  // 습도 값 수신
      int d = Wire.read();  // 미세먼지 값 수신

      Serial.print("T: "); Serial.print(t);
      Serial.print(" H: "); Serial.print(h);
      Serial.print(" Dust: "); Serial.println(d);
      btSerial.print(t);
      btSerial.print(" ");
      btSerial.print(h);
      btSerial.print(" ");
      btSerial.println(d);
      Serial.println("Send complete");
    }
    
    i2cRequestPrevMillis = now; // 요청 후 시간 갱신
  }
}


bool lastButtonState = HIGH;
void handleBELL() {
  int currentButtonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    enqueueBT(34);
  }
  lastButtonState = currentButtonState;
}

void handleFlame() {
  int value = digitalRead(flameSensor);
  
  if (value == HIGH ) {
    Serial.println("불꽃 감지");
    Serial.println(value);
    enqueueBT(90);
  }
}

void handlePhotoSensor() {
  int light = analogRead(photoSensor);
  digitalWrite(ledGate, light < 500 ? LOW : HIGH);
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
      garagePos += garageStep;
      if (garagePos > 120) garagePos = 120;
      garageServo.write(garagePos);
      if (garagePos == 120) {
        garageOpening = false;
        garageOpenedFlag = true;
        garageOpenedMillis = now;
        digitalWrite(ledGarage, HIGH);
        garageLedOn = true;
        garageLedMillis = now;
      }
    } else if (garageClosing && garagePos > 0) {
      garagePos -= garageStep;
      if (garagePos < 0) garagePos = 0;
      garageServo.write(garagePos);
      if (garagePos == 0) {
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
    recentGarageActionMillis = now;
  }

  if (garageLedOn && (now - garageLedMillis >= 30000)) {
    digitalWrite(ledGarage, LOW);
    garageLedOn = false;
  }
}

void checkUltrasonic() {
  unsigned long now = millis();
  if (garageOpening || garageClosing || garagePos != 0 || (now - recentGarageActionMillis < garageCooldown)) return;

  long dist = measureDistance(trig2, echo2);
  if (dist > 0 && dist < 3) {
    garageOpening = true;
    recentGarageActionMillis = now;
  }
}

void checkUltrasonic2() {
  unsigned long now = millis();
  if (garageOpening || garageClosing || garagePos != 0 || (now - recentGarageActionMillis < garageCooldown)) return;

  long dist = measureDistance(trig1, echo1);
  if (dist > 0 && dist < 3) {
    garageOpening = true;
    recentGarageActionMillis = now;
    Serial.println(dist);
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
    if (btSerial.available() >= 2) { // 2바이트 데이터가 수신되었는지 확인
    byte lowByte = btSerial.read();     // 첫 번째 바이트 (하위 바이트)
    byte highByte = btSerial.read();    // 두 번째 바이트 (상위 바이트)

    // 2바이트를 하나의 16비트 정수로 조립 (Signed Integer 처리)
    int16_t receivedValue = (int16_t)(lowByte | (highByte << 8));

    Serial.print("Received 2-byte value: ");
    Serial.println(receivedValue);

    Wire.beginTransmission(SLAVE_ADDRESS);
    Wire.write(lowByte);
    Wire.write(highByte);
    Wire.endTransmission();
    

    // 값 처리 로직 추가...
    }
    else{
      byte singleCmd = btSerial.read();
      switch (singleCmd) {
        case 1: openDoorLock(); break;
        case 2: closeDoorLock(); break;
        case 3: digitalWrite(ledGarage, HIGH); break;
        case 4: digitalWrite(ledGarage, LOW); break;
        case 5: garageOpening = true; garageClosing = false; recentGarageActionMillis = millis(); break;
        case 6: garageClosing = true; garageOpening = false; recentGarageActionMillis = millis(); break;
        default:
          // I2C를 통해 단일 명령 전송
          Wire.beginTransmission(SLAVE_ADDRESS);
          Wire.write(singleCmd);
          Wire.endTransmission();
          break;
      }
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

void enqueueBT(byte value) {
  if ((bufferEnd + 1) % MAX_BUFFER_SIZE != bufferStart) {
    sendBuffer[bufferEnd] = value;
    bufferEnd = (bufferEnd + 1) % MAX_BUFFER_SIZE;
  }
}

void handleBluetoothSending() {
  unsigned long now = millis();
  if (now - btSendPrevMillis >= btSendInterval && bufferStart != bufferEnd) {
    btSendPrevMillis = now;
    byte value = sendBuffer[bufferStart];
    btSerial.println(value);
    Serial.print("Sent: ");
    Serial.println(value);
    bufferStart = (bufferStart + 1) % MAX_BUFFER_SIZE;
  }
}
