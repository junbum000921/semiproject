//PART 3
// 앱,리모컨 제어X 독립적임.
// 주차확인(초음파, OLED)
// 현관제어(PIR, LED)
// 주방제어(후드(DC),)
// 가스누출 시 밸브잠금


#include <U8glib.h>           //u8g 라이브러리 추가 ( OLED 화면 )
#include <Servo.h>            //서보모터 라이브러리 추가

Servo servo3;                  //부엌 창문 서보모터 객체 생성
Servo servo4;                  //가스 밸브 서보모터 객체 생성


#define ECHO3 6                 //ECHO, 6번
#define TRIG3 8                 //TRIG, 8번
#define LED_DOOR 2              //현관 LED 제어, 2번 핀
#define TOUCH2 4                  //터치 제어, 4번 핀
#define SERVO_WIN 3               //서보모터 각도 제어, 3번 핀으로 설정
#define SERVO_GAS 5               //서보모터 각도 제어, 5번 핀으로 설정

#define HOOD_MOTOR 7                //DC 모터 제어, 7번 핀
#define PIR 9                      //인체감지 제어, 9번
#define GAS_OUT A0                //가스센서 아날로그 출력, A0핀



U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);      //SSD1306 128X64 I2C 규격 선택  ( OLED 화면 )




bool hoodState = false;                       //후드 상태 저장 변수
bool servo3State = false;                      //서보모터 상태 저장 변수
static bool carDetected = false;              //차량 감지 상태 저장
int motor_value = 0;

unsigned long displayStartTime = 0;           // 화면 표시 시작 시간
//int value = 0;                          //터치센서 스위칭 횟수

int start_time = millis(); //setup위에 넣기
int led_on_time;

void setup() {
  Serial.begin(9600);



  servo3.attach(SERVO_WIN);    //부엌 창문 서보 모터 제어 위한 핀 설정
  servo4.attach(SERVO_GAS);    //가스밸브 서보 모터 제어 위한 핀 설정
  servo3.write(175);        //처음 창문 위치 설정
  servo4.write(90);        //처음에 가스 배관과 일직선상에 위치

  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);
  pinMode(TOUCH2, INPUT);
  pinMode(HOOD_MOTOR, OUTPUT);

  pinMode(PIR, INPUT);
  pinMode(LED_DOOR, OUTPUT);
  pinMode(GAS_OUT, INPUT);

  delay(200);
  servo3.detach();
  servo4.detach();

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_fub14);
    u8g.setPrintPos(20, 40);
    u8g.print(" ");
  } while (u8g.nextPage());     //화면 빈 화면으로 초기화(즉시 꺼짐)

}





void loop() {
//변수 설정
  int gasValue = analogRead(GAS_OUT);           //아날로그데이터 읽어 gasValue 저장(가스센서 신호)
  int pir_value = digitalRead(PIR);        //사람 감지해서 현관 등 제어
  int touchState = digitalRead(TOUCH2);         //터치 감지해서 후드, 창문 제어


//주방 환기- 후드(수동) & 창문
  if (touchState == HIGH) {   //터치했을 때
    Serial.println("1");      //시리얼 모니터로 상태 확인용
    motor_value = (motor_value==0) ? 200 : 0;     
    analogWrite(HOOD_MOTOR, motor_value);  // 후드 ON/OFF
          
    //창문 OPEN/CLOSE
    servo3.attach(SERVO_WIN);
    servo3State = !servo3State;
    servo3.write(servo3State ? 115 : 175);
    delay(500);
    servo3.detach();
        
    }

//초음파 준비
  digitalWrite(TRIG3, HIGH);           //펄스파 발생
  delayMicroseconds(10);              //10us 지연
  digitalWrite(TRIG3 ,LOW);            //펄스파 종료

  long duration = pulseIn(ECHO3, HIGH);          //Echo 핀이 high된 이후 지연된 펄스의 시간: duration
  if(duration == 0) { return; }                 //시간 0이면 종료

  long distance = duration / 58;                //총 걸리는 시간 58로 나눠 cm 단위로 거리 측정.

  Serial.print("Distance : ");    //시리얼 모니터로 거리 데이터 출력
  Serial.print(distance);
  Serial.println("cm");
  delay(1000);        //거리 데이터 2초마다 출력


  //OLED -입차 신호
  if (distance <= 6 && !carDetected) { 
    carDetected = true;
    displayStartTime = millis();    //현재 시간 저장

    u8g.firstPage();    
    do {
        u8g.setFont(u8g_font_fub14);                //폰트
        u8g.setPrintPos(20, 40);                    //글 위치
        u8g.print("PARKING");                       //입차 중이라 표시
    } while(u8g.nextPage());
} 


  // 5초 후 화면 끄기
      if (carDetected && millis() - displayStartTime >= 5000) {
        carDetected = false;  // 다시 감지할 수 있도록 초기화

        u8g.firstPage();
        do {
            u8g.setFont(u8g_font_fub14);            //폰트
            u8g.setPrintPos(40, 55);                //글 위치
            u8g.print(" ");  // 빈 화면 출력
        } while (u8g.nextPage());
    }


//현관

if(pir_value == HIGH){  //움직임 감지 시
        digitalWrite(LED_DOOR, HIGH);   
        led_on_time = millis();
        Serial.println("Detected");
  
    }
    if((millis()-led_on_time)>=2000){
        digitalWrite(LED_DOOR,LOW);
        led_on_time = 0;      //타이머 초기화
    }  


//주방
  if(gasValue >=150)      //가스 누출 상황일 시
  {
 

    servo4.attach(SERVO_GAS);                             //서보모터 출력
    servo4.write(180);       //서보모터와 가스배관과 90도 방향되도록 회전
    delay(500);               //서보모터가 위치까지 도달할 수 있도록 1초 대기
    servo4.detach();         //서보모터 출력 신호 정지
  }

  
}
