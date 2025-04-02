#define PIEZO 12
#define BUTTON 8
int value = 0;

void setup() {
  pinMode(BUTTON, INPUT);
  Serial.begin(9600);
}

void loop() {
  value = digitalRead(BUTTON);

  if (value == LOW) { // 버튼을 누르면
    Serial.println("Button Pressed");
    tone(PIEZO, 1000); // '띵' 소리
    delay(300);
    tone(PIEZO, 800);  // '동' 소리
    delay(1000);
    noTone(PIEZO);     // 소리 끄기
  }

  delay(100); // 버튼 중복 입력 방지
}
