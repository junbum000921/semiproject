// #include <Servo.h>
// #include <U8glib.h>
// #include "DHT.h"
// #include <SoftwareSerial.h>
// #include <IRremote.h>

#define PIEZO_livingroom 3
#define LED_toilet 4
#define LED_door 5
#define MOTOR 6
#define DHT 7
#define LED_livingroom 8
#define PIEZO_room1 9
#define SERVO_window 10
#define SERVO_curtain 11 
#define TXD 12
#define RXD 13
#define OUTPUT_VOLTAGE A0
#define REMOTE A1
#define LED_room1 A2
#define PIR A3
#define INPUT_PULSE A4
#define DHTTYPE DHT11

SoftwareSerial mySerial(TXD,RXD);
DHT dht(DHT, DHTTYPE);
Servo servo1;
Servo servo2;
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
IRrecv irrecv(REMOTE);
decode_results results;

float preVoltage = 0; //0~1023 범위의 출력 전압
float voltage = 0; //0~5 범위의 출력 전압c:\Users\kosta\Desktop\080215\15장(블루투스 모듈)\practice_15\practice_15.ino
float dustDensity = 0; //미세먼지 농도 수치 
float sumDustDensity = 0; //미세먼지 농도 수치 합
float avgDustDensity = 0;
int motor_value = 0;
int servo_window_value = 0;
int servo_curtain_value = 0;


void setup(){
    servo1.attach(SERVO_window);
    servo2.attach(SERVO_curtain);
    pinMode(PIEZO_livingroom, OUTPUT);
    pinMode(LED_toilet, OUTPUT);
    pinMode(LED_door, OUTPUT);
    pinMode(MOTOR, OUTPUT);
    pinMode(LED_livingroom, OUTPUT);
    pinMode(PIEZO_room1, OUTPUT);
    pinMode(TXD, OUTPUT);
    pinMode(RXD, INPUT);
    pinMode(LED_room1, OUTPUT);
    pinMode(PIR, INPUT);
    pinMode(INPUT_PULSE, INPUT);
    irrecv.enableIRIn();
    dht.begin();
    Serial.begin(9600);
    mySerial.begin(9600);
}


void loop(){

    int pir_value = digitalRead(PIR);        //사람 감지해서 현관 등 제어
    if(pir_value == 1){
        digitalWrite(LED_door, HIGH);       
    }
    
    float humidity = dht.readHumidity();         //온습도,미세먼지 측정
    float temperature = dht.readTemperature();

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
    if(isnan(humidity) || isnan(temperature)){
        return;
    }

    mySerial.print(temperature,1);     //온습도, 미세먼지 앱 출력
    mySerial.print(" "); 
    mySerial.print(humidity,1); 
    mySerial.print(" "); 
    mySerial.println(avgDustDensity,1); 

    if(mySerial.available()){               //앱 연결
        byte input = mySerial.read();

        switch(input){
            case 11:                               //거실 등 켜고 끔
                digitalWrite(LED_livingroom,HIGH); 
                break;
            case 12:
                digitalWrite(LED_livingroom,LOW);
                break;
            case 13:                               //방1 등 켜고 끔
                digitalWrite(LED_room1,HIGH);
                break;
            case 14:
                digitalWrite(LED_room1,LOW);
                break;
            case 15:                               //화장실 등 켜고 끔
                digitalWrite(LED_toilet,LOW);
                break;
            case 16:
                digitalWrite(LED_toilet,LOW);
                break;
            case 17:                               //실링팬 제어
                analogWrite(MOTOR, 0);
                break;
            case 18:
                analogWrite(MOTOR, 150);
                break;
            case 19:                              //창문 제어  TODO:
                analogWrite(SERVO_window, );
                break;
            case 20:                              
                analogWrite(SERVO_window, );
                break;
            case 21:                             //커튼 제어  TODO:
                analogWrite(SERVO_curtain, );
                break;
            case 22:                           
                analogWrite(SERVO_curtain, );
                break;
        }
    }

    if (irrecv.decode(&results)){        //리모컨 조작
        switch (results.value){
            case 0xFF6897:         // 1 거실등 조작
                digitalwrite(LED_livingroom, !digitalRead(LED_livingroom));
                break;
            case 0xFF9867:         // 2 방1 등 조작
                digitalWrite(LED_room1, !digitalRead(LED_room1));
                break;
            case 0xFFB04F:         // 3 방2 등 조작(다른 보드)
                mySerial.println("23");
                break;
            case 0xFF30CF:         // 4 화장실 등 조작
                digitalWrite(LED_toilet, !digitalRead(LED_toilet));
                break;
            case 0xFF18E7:         // 5 실링팬 조작
                motor_value = (motor_value == 0) ? 150 : 0;
                analogWrite(MOTOR, motor_value);
                break;
            case 0xFF7A85:          // 6 후드 조작(다른 보드)
                mySerial.println("24");
                break;
            case 0xFF5AA5:          // 7 도어락 제어(다른 보드)
                mySerial.println("25");
                break;

            case 0xFFC23D:         // -> 창문 열기   TODO: 서보모터 각도 확인
                servo_window_value += 10;
                analogWrite(SERVO_window, servo_window_value);
                break;
            case 0xFF22DD:         // <- 창문 닫기   TODO: 서보모터 각도 확인
                servo_window_value -= 10;
                analogWrite(SERVO_window, servo_window_value);
                break;
                
            case 0xFF629D:         // ^ 커튼 걷기   TODO: 서보모터 각도 확인
                servo_curtain_value += 10;
                analogWrite(SERVO_curtain, servo_curtain_value);
                break;    
            case 0xFFA857:         // v 커튼 치기   TODO: 서보모터 각도 확인
                servo_curtain_value -= 10;
                analogWrite(SERVO_curtain, servo_curtain_value);
                break;  
        }   
        irrecv.resume();
    }
}












