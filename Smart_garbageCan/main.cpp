//Linh kiện cần thiết
//Arduino UNO, Servo sg90, cảm biến siêu âm sr04
#include <Arduino.h>
#include <Servo.h>

//set up servo
Servo servo;
const int servoPin = 9;// chân signal servo nối với chân số 9 trên arduino
const int openAngle = 0;
const int closeAngle = 90;

//set up SR04
const int triPin = 5; //chân trigger nối với chân số 5
const int EchPin = 6; //chân echo nối với chân số 6
long khcach, khcach_tb;
long averDist[3];
const int nguong_khcach = 20; //don vi cm
float readDistance();
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(triPin,OUTPUT);
  pinMode(EchPin, INPUT);

  servo.attach(servoPin);
  servo.write(closeAngle);
  delay(100);
  servo.detach();
}

void loop() {
  // put your main code here, to run repeatedly:
  //do khoang cach 3 lan
  for(int i = 0; i <=2; i++){
    khcach = readDistance();
    averDist[i] = khcach;
    delay(10);
  }
  // tinh khoang cach trung binh
  khcach_tb = (averDist[0] + averDist[1] + averDist[2]) / 3;
  Serial.println(khcach_tb);
  //dieu khien servo
  if (khcach_tb <= nguong_khcach){
    servo.attach(servoPin);
    delay(1);
    servo.write(openAngle);
    delay(3500);
  }else{
    servo.write(closeAngle);
    delay(1000);
    servo.detach();
  }
}
float readDistance(){
  digitalWrite(triPin,LOW);
  delayMicroseconds(2);
  digitalWrite(triPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(triPin,LOW);
  //Đo độ rộng xung của chân phản hồi và tính giá trị khoảng cách
  float khcach = pulseIn(EchPin, HIGH) / 58.00; // cong thuc: (340m/s * 1us) / 2
  return khcach;
}
//ham doc gia tri cua hr04 va tinh khoang cach

// put function definitions here:

