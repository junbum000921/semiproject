#include <IRremote.h>
#include <Servo.h>

#define RECV_PIN 4
#define SERVO_PIN 6

#define BUTTON_OK 0xFF02FD
#define BUTTON_UP 0xFF629D
#define BUTTON_DOWN 0xFFA857

Servo servo;
IRrecv irrecv(RECV_PIN);
decode_results results;

bool isOn = false;
int angle = 90; // 초기 서보모터 각도
unsigned long lastCommand = 0;

void setup() {
    Serial.begin(9600);
    irrecv.enableIRIn(); // IR 리모컨 수신 활성화
    servo.attach(SERVO_PIN);
    servo.write(angle);
}

void loop() {
    if (irrecv.decode(&results)) {
        unsigned long command = results.value;
        
        // 반복 신호 무시
        if (command == 0xFFFFFFFF) {
            irrecv.resume();
            return;
        }
        
        // 중복 입력 방지
        /*if (command == lastCommand) {
            irrecv.resume();
            return;
        }*/
        lastCommand = command;
        
        Serial.println(command, HEX);

        if (command == BUTTON_OK) {
            isOn = !isOn;
            Serial.println(isOn ? "Power ON" : "Power OFF");
        }
        
        if (isOn) {
            if (command == BUTTON_UP) {
                angle +=30;
                if(angle>=180){
                  angle=180;
                }
                
            }
            else if (command == BUTTON_DOWN) {
                angle -=30;
                if(angle<=0){
                  angle=0;
                }
                
            }
            servo.write(angle);
        }
        
        irrecv.resume(); // 다음 신호 수신
    }
    delay(10);
}