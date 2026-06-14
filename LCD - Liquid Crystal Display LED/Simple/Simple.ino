#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

String line1 = "Wicky's here!              Please subscribe!";
String line2 = "Welcome to my channel!     And like!";

int pos = 0;

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(pos, pos + 16));

  lcd.setCursor(0, 1);
  lcd.print(line2.substring(pos, pos + 16));

  pos++;

  if (pos > line2.length() - 16) {
    pos = 0;
  }

  delay(300);
}