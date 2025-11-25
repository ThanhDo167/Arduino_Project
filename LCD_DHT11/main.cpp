#include <DHT.h>
#include <DHT_U.h>
#define Type DHT11
int sensor = 8;

DHT HT(sensor, Type);
float hum;
float tempC;
float tempF;
int dt (500);

//setting lcd
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // put your setup code here, to run once:
  //DHT11
  Serial.begin(9600);
  HT.begin();

  //LCD
  lcd.begin(16,2);


}

void loop() {
  // put your main code here, to run repeatedly:
  //do am
  hum = HT.readHumidity();
  Serial.print("Do am: ");
  Serial.print(hum);

  //nhiet do
  tempC = HT.readTemperature();
  Serial.print(" Nhiet do(C): ");
  Serial.print(tempC);

  tempF = HT.readTemperature(true);
  Serial.print(" Nhiet do(F): ");
  Serial.println(tempF);

  delay(dt);
  //hien thi tren lcd
  lcd.setCursor(0,0);
  delay(dt);
  lcd.print("Do am: ");
  lcd.print(hum);
  

  lcd.setCursor(0,1);
  delay(dt);
  lcd.print("Nhiet do: ");
  lcd.print(tempC);
  lcd.print(" *C");
}
