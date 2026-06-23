#include <RCSwitch.h>

RCSwitch rf = RCSwitch();
const int ledPin = 8;
bool ledState = false;
unsigned long ledTimer = 0;
const unsigned long ledDuration = 500;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  rf.enableReceive(digitalPinToInterrupt(2));
  Serial.println("433 MHz receiver ready");
}

void loop() {
  if (rf.available()) {
    digitalWrite(ledPin, HIGH);
    ledState = true;
    ledTimer = millis();
    
    Serial.print("Code: ");
    Serial.println(rf.getReceivedValue());

    Serial.print("Bit length: ");
    Serial.println(rf.getReceivedBitlength());

    Serial.print("Protocol: ");
    Serial.println(rf.getReceivedProtocol());

    Serial.print("Delay: ");
    Serial.println(rf.getReceivedDelay());

    Serial.println("----------------");

    rf.resetAvailable();
  }
  
  if (ledState && (millis() - ledTimer >= ledDuration)) {
    digitalWrite(ledPin, LOW);
    ledState = false;
  }
}