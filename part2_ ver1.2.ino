#include <Servo.h>
#include <U8glib.h>
#include "DHT.h"
#include <SoftwareSerial.h>
#include <IRremote.h>

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

SoftwareSerial mySerial(TXD,RXD);
DHT dht(DHT_PIN, DHTTYPE);
Servo servo1;
Servo servo2;
IRrecv irrecv(REMOTE);
decode_results results;

float preVoltage = 0; //0~1023 범위의 출력 전압
float voltage = 0; //0~5 범위의 출력 전압c:\Users\kosta\Desktop\080215\15장(블루투스 모듈)\practice_15\practice_15.ino
float dustDensity = 0; //미세먼지 농도 수치 
float sumDustDensity = 0; //미세먼지 농도 수치 합
float avgDustDensity = 0;
int servo_window_value = 0;
int servo_curtain_value = 0;
int motor_value = 0;
int led_on_time;
int start_time = millis();

void setup(){
    servo1.attach(SERVO_window);
    servo1.write(90);
    servo1.detach();
    servo2.attach(SERVO_curtain);
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
    mySerial.begin(9600);
    irrecv.enableIRIn();
}

void LED_control(int which_led){
    if(which_led == 1){
        digitalWrite(LED_livingroom, !digitalRead(LED_livingroom));
    }
    if(which_led == 2){
        digitalWrite(LED_room1, !digitalRead(LED_room1));
    }
    if(which_led == 3){
        digitalWrite(LED_room2, !digitalRead(LED_room2));
    }
    if(which_led == 4){
        digitalWrite(LED_toilet, !digitalRead(LED_toilet));
    }
}

void FAN_control(){
    motor_value = (motor_value == 0) ? 150 : 0;
    analogWrite(MOTOR, motor_value);
}

void WINDOW_control(int open_close){
    servo1.attach(SERVO_window);
    if(open_close == 1){
        /*for(int i=0; i<10; i++){
            servo_window_value+=9;
            if(servo_window_value>178) servo_window_value=178;
            servo1.write(servo_window_value);
            delay(100);
        }*/
        servo_window_value = 177;
        servo1.write(servo_window_value);
        delay(10);
    }
    else {
        /*for(int i=0; i<10; i++){
            servo_window_value-=9;
            if(servo_window_value<90) servo_window_value=90;
            servo1.write(servo_window_value);
            delay(100);
        }*/
        servo_window_value = 90;
        servo1.write(servo_window_value);
        delay(10);
    }
    servo1.detach();
}

void Send_Data(float t, float h, float d){
  mySerial.print(t);
  mySerial.print(" ");
  mySerial.print(h);
  mySerial.print(" ");
  mySerial.println(d);
  Serial.println("Send complete");
}

void loop(){

    float humidity = dht.readHumidity();         //온습도,미세먼지 측정
    float temperature = dht.readTemperature();

    if(millis()%10000==0){
      sumDustDensity = 0;
      for(int i=0;i<30;i++){
        digitalWrite(INPUT_PULSE, LOW);
        delayMicroseconds(280);
        preVoltage = analogRead(OUTPUT_VOLTAGE);
        delayMicroseconds(40);
        digitalWrite(INPUT_PULSE, HIGH);
        delayMicroseconds(9680);
        voltage = preVoltage * 5.0 / 1024.0;
        dustDensity = (voltage-0.3)/0.005;
        sumDustDensity += dustDensity;
        delay(10);
      }
      avgDustDensity = sumDustDensity / 30.0;
    }
    
    if(isnan(humidity) || isnan(temperature)){
        return;
    }

    if((millis()%2000)==0){               //온습도 미세먼지 출력
      Send_Data(temperature, humidity, avgDustDensity);
      Serial.print(temperature,1);    
      Serial.print(" "); 
      Serial.print(humidity,1);
      Serial.print(" "); 
      Serial.println(avgDustDensity,1);
      delay(10);
      mySerial.flush();
    }
  
    if(mySerial.available()){               //앱 연결
        byte input = mySerial.read();
        switch (input){
            case 7:         //거실등 조작
                LED_control(1);              
                break;
            case 8:         //방1 등 조작
                LED_control(2);
                break;
            case 9:         //방2 등 조작
                LED_control(3);
                break;
            case 10:         //화장실 등 조작
                LED_control(4);
                break;
            case 11:         //실링팬 조작
                FAN_control();
                break;
            case 12:         //창문 제어
                if(servo_window_value <=120){
                    WINDOW_control(1);
                }
                else {
                    WINDOW_control(2);
                }
                break;
        }
    }    

  if (irrecv.decode(&results)){        //리모컨 조작
      switch (results.value){
          case 0xFF6897:         // 1 거실등 조작
              LED_control(1);              
              break;
          case 0xFF9867:         // 2 방1 등 조작
              LED_control(2);
              break;
          case 0xFFB04F:         // 3  방2 등 조작
              LED_control(3);
              break;
          case 0xFF30CF:         // 4 화장실 등 조작
              LED_control(4);
              break;
          case 0xFF18E7:         // 5 실링팬 조작
              FAN_control();
              break;
          case 0xFFC23D:         // -> 창문 열기   
              WINDOW_control(1);
              break;
          case 0xFF22DD:         // <- 창문 닫기   
              WINDOW_control(2);
              break;
          case 0xFF629D:         // ^ 커튼 올리기   
              for(int i=0; i<20; i++){
                servo_window_value+=9;
                if(servo_curtain_value>177) servo_curtain_value=177;
                servo2.write(servo_curtain_value);
                delay(10);
              }
              break;
          case 0xFFA857:         // v 커튼 내리기
              for(int i=0; i<20; i++){
                  servo_curtain_value-=9;
                  if(servo_curtain_value<3) servo_curtain_value=3;
                  servo2.write(servo_curtain_value);
                  delay(10);
              }
              break;
      }
      irrecv.resume();
  }
}
