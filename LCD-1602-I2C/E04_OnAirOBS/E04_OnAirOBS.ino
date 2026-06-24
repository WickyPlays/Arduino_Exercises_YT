#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool isOnAir = false;
String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
    
  lcd.init();
  lcd.clear();
  lcd.noBacklight();
  
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("v1.0");
  delay(1500);
  lcd.clear();
  lcd.noBacklight();
  
  displayStatus(false);
  
  Serial.println("READY");
}

void loop() {

  while (Serial.available()) {
    char inChar = (char)Serial.read();
    Serial.print("CHAR:");
    Serial.println((int)inChar);

    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
  
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
  
  delay(10);
}

void processCommand(String command) {
  command.trim();
  command.toUpperCase();
  
  if (command == "ON") {
    isOnAir = true;
    displayStatus(true);
    Serial.println(" ON AIR");
  } 
  else if (command == "OFF") {
    isOnAir = false;
    displayStatus(false);
    Serial.println("OFF AIR");
  }
  else if (command.startsWith("R")) {
    int r = command.substring(1, 2).toInt();
    int g = command.substring(2, 3).toInt();
    int b = command.substring(3, 4).toInt();
    if (r == 1 && g == 0 && b == 0) {
      isOnAir = true;
      displayStatus(true);
    } else {
      isOnAir = false;
      displayStatus(false);
    }
  }
  else {
    Serial.println("INVALID");
  }
}

void displayStatus(bool onAir) {
  lcd.clear();
  
  if (onAir) {
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("    ON AIR  ");
    lcd.setCursor(0, 1);
    lcd.print("    RECORDING   ");
  } 
  else {
    lcd.noBacklight();
    lcd.setCursor(0, 0);
    lcd.print("    STAND BY    ");
    lcd.setCursor(0, 1);
    lcd.print("   NOT RECORD  ");
  }
}