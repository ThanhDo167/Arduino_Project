#include <Arduino.h>
#define BUTTON_PIN 10

void setup() {
  for (int i=2; i<=9; i++){ //vòng lặp qua các chân từ 0 - 6
    pinMode(i, OUTPUT); //cấu hình chân cho chân i là đầu ra 
    digitalWrite(i,HIGH); // xuất tín hiệu chân mức cao
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);

}

void blink_3s(){ // hàm led nháy 
  for (int i=2; i<=5; i++){ //vòng lặp qua các chân từ 0-4
    if (digitalRead(BUTTON_PIN)== HIGH) return ;
    pinMode(i, OUTPUT);
    digitalWrite(i+5, LOW);
    digitalWrite(i,LOW);
    delay(1000); //delay trong 1 giây
  }
  for (int i=5; i<=9; i++){// vòng lặp qua các chân từ 4 đến 7
    pinMode(i, OUTPUT);
    digitalWrite(i-5, HIGH);
    digitalWrite(i,HIGH);
    delay(1000); //delay trong 1 giây
  }
}
void loop() {
  if(digitalRead(BUTTON_PIN)== LOW){
    blink_3s();
  }
  
}

