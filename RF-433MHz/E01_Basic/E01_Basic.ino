#include <RCSwitch.h>

RCSwitch rf = RCSwitch();

void setup() {
  Serial.begin(115200);
  rf.enableReceive(digitalPinToInterrupt(2));
  Serial.println("433 MHz receiver ready");
}

void loop() {
  if (rf.available()) {
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
}