///스케쥴링 없는버전
#include <SoftwareServo.h>
#include <U8glib.h>
#include "DHT.h"
#include <IRremote.h>
#include <NewTone.h>
#include <Wire.h>

#define LED_room2 2
#define PIEZO_livingroom 3
#define LED_toilet 4
#define PIEZO_room1 5
#define MOTOR 6
#define DHT_PIN 7
#define LED_livingroom 8
#define INPUT_PULSE 9
#define SERVO_window 10
#define SERVO_curtain 11 
#define LED_door 12
#define PIEZO_room2 13
#define OUTPUT_VOLTAGE A0
#define REMOTE A1
#define LED_room1 A2
#define PIR A3
#define TXD A4
#define RXD A5
#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);
SoftwareServo servo1;
SoftwareServo servo2;
IRrecv irrecv(REMOTE);
decode_results results;

int input=0;
float preVoltage = 0;
float voltage = 0;
float dustDensity = 0;
float sumDustDensity = 0;
float avgDustDensity = 0;
int servo_window_value = 0;
int servo_curtain_value = 0;
int motor_value = 0;
#define SLAVE_ADDRESS 2
void setup(){
    servo1.attach(SERVO_window);
    servo1.write(90);
    for (int i = 0; i < 20; i++) {
        SoftwareServo::refresh();
        delay(10);
    }
    servo1.detach();

    servo2.attach(SERVO_curtain);
    servo2.write(90);

    pinMode(PIEZO_livingroom, OUTPUT);
    pinMode(LED_toilet, OUTPUT);
    pinMode(LED_door, OUTPUT);
    pinMode(MOTOR, OUTPUT);
    pinMode(LED_livingroom, OUTPUT);
    pinMode(PIEZO_room1, OUTPUT);
    pinMode(LED_room1, OUTPUT);
    pinMode(LED_room2, OUTPUT);
    pinMode(PIR, INPUT);
    pinMode(INPUT_PULSE, OUTPUT);
    pinMode(OUTPUT_VOLTAGE, INPUT);

    dht.begin();
    Serial.begin(9600);
    irrecv.enableIRIn();
    Wire.begin(SLAVE_ADDRESS);  // 슬레이브로서 I2C 초기화
    Wire.onRequest(requestEvent);  // 요청이 오면 데이터를 보내는 이벤트 설정
    Wire.onReceive(receiveEvent);  // 마스터가 데이터를 보낼 때 호출될 함수 등록

    delay(1000);
}

void handlePIEZO(int num, int cmd){
    if(cmd == 1){
    NewTone(num, 1800, 1000);   // 500Hz
    delay(1000);               // 1초 기다려줘야 다음 톤이 안 겹쳐짐

    NewTone(num, 1500, 1000);  // 1000Hz
    delay(1000);               // 이것도 마찬가지

    noNewTone(num);
}
    if(cmd == 2){
      for(int i=0; i<500; i++){
        NewTone(num, 1500+i, 10);
        delay(5);
      }
      for(int i=0; i<500; i++){
        NewTone(num, 2000-i, 10);
        delay(5);
      }
        
    }
}

void LED_control(int which_led){
    if(which_led == 1) digitalWrite(LED_livingroom, !digitalRead(LED_livingroom));
    if(which_led == 2) digitalWrite(LED_room1, !digitalRead(LED_room1));
    if(which_led == 3) digitalWrite(LED_room2, !digitalRead(LED_room2));
    if(which_led == 4) digitalWrite(LED_toilet, !digitalRead(LED_toilet));
}

void FAN_control(){
    motor_value = (motor_value == 0) ? 150 : 0;
    analogWrite(MOTOR, motor_value);
}

void WINDOW_control(int open_close){
    servo1.attach(SERVO_window);
    if(open_close == 1){
        servo_window_value = 177;
        servo1.write(servo_window_value);
    } else {
        servo_window_value = 90;
        servo1.write(servo_window_value);
    }

    for (int i = 0; i < 20; i++) {
        SoftwareServo::refresh();
        delay(10);
    }

    servo1.detach();
}

float temperature=0;
float humidity=0;
int t_i=0;
int h_i=0;
int d_i=0;

void loop(){
    SoftwareServo::refresh();  // 꼭 필요함!

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if(millis() % 3000 == 0){
        sumDustDensity = 0;
        for(int i = 0; i < 30; i++){
            digitalWrite(INPUT_PULSE, LOW);
            delayMicroseconds(280);
            preVoltage = analogRead(OUTPUT_VOLTAGE);
            delayMicroseconds(40);
            digitalWrite(INPUT_PULSE, HIGH);
            delayMicroseconds(9680);
            voltage = preVoltage * 5.0 / 1024.0;
            dustDensity = (voltage - 0.3) / 0.005;
            sumDustDensity += dustDensity;
            delay(10);
        }
        avgDustDensity = sumDustDensity / 30.0;
        
        t_i=int(temperature);
        h_i=int(humidity);
        d_i=int(avgDustDensity);

        Serial.println(t_i);
        Serial.println(h_i);
        Serial.println(d_i);
    }

    switch(input){
        case 7: LED_control(1); break;
        case 8: LED_control(2); break;
        case 9: LED_control(3); break;
        case 10: LED_control(4); break;
        case 11: FAN_control(); break;
        case 12:
            if(servo_window_value <= 120) WINDOW_control(1);
            else WINDOW_control(2);
            break;
        case 13: handlePIEZO(PIEZO_livingroom, 1); break;
        case 14: handlePIEZO(PIEZO_livingroom, 2); break;
        case 15: handlePIEZO(PIEZO_room1, 2); break;
        case 16: handlePIEZO(PIEZO_room2, 2); break;
    }
    input=0;

    if (irrecv.decode(&results)){
        switch (results.value){
            case 0xFF6897: LED_control(1); break;
            case 0xFF9867: LED_control(2); break;
            case 0xFFB04F: LED_control(3); break;
            case 0xFF30CF: LED_control(4); break;
            case 0xFF18E7: FAN_control(); break;
            case 0xFFC23D: WINDOW_control(1); break;
            case 0xFF22DD: WINDOW_control(2); break;
            case 0xFF629D:
                for(int i = 0; i < 20; i++){
                    servo_curtain_value += 9;
                    if(servo_curtain_value > 177) servo_curtain_value = 177;
                    servo2.write(servo_curtain_value);
                    SoftwareServo::refresh();
                    delay(10);
                }
                break;
            case 0xFFA857:
                for(int i = 0; i < 20; i++){
                    servo_curtain_value -= 9;
                    if(servo_curtain_value < 3) servo_curtain_value = 3;
                    servo2.write(servo_curtain_value);
                    SoftwareServo::refresh();
                    delay(10);
                }
                break;
        }
        irrecv.resume();
    }
}
void requestEvent() {
  Wire.write(t_i);  // 온도 값 전송
  Wire.write(h_i);  // 습도 값 전송
  if(d_i<0){
    d_i=0;
  }
  
  Wire.write(d_i);  // 미세먼지 값 전송
}
int scheduleInputs[6] = {0, 0, 0, 0, 0, 0}; // 스케쥴링을 위한 6개 변수

void receiveEvent(int numBytes) {//스케쥴링 변수 받기 위해 두개의 케이스로 분류.
    if (numBytes == 1) {
        // 1개의 값만 수신된 경우
        input = Wire.read();
    } else if (numBytes == 6) {
        // 6개의 값이 수신된 경우
        for (int i = 0; i < 6; i++) {
            if (Wire.available()) {
                scheduleInputs[i] = Wire.read();
            }
        }
    }
}