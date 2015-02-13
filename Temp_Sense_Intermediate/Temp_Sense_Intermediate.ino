
// This code is sparse in comments; check the document "Code_Explanations" or of a similar name for an explanation.
//----------------------------------------------------------------Part 1


int i = 0;
int highestTemp = 0;
int highTemp1 = 0;
int highTemp2 = 0;
unsigned long timenotGottenSerial1 = 0;
unsigned long timenotGottenSerial2 = 0;
unsigned long serial1Absence = 0;
unsigned long serial2Absence = 0;
boolean notGottenSerial1 = false;
boolean notGottenSerial2 = false;
#include <Wire.h>
#include <SoftwareSerial.h>
SoftwareSerial tempSlave1(8, 9); // RX, TX
SoftwareSerial tempSlave2(10, 11); // RX, TX
unsigned long loopCounter = 0;

void setup() {
  
  Serial.begin(9600);
  tempSlave1.begin(9600); // might use hardware serial to recieve data and I2C to send data instead
  tempSlave2.begin(9600); // of two software-serials
}
//----------------------------------------------------------------Part 2
void loop() {
  if (millis() > loopCounter) {
  loopCounter = millis() + 100; // runs 10 times/second
  highestTemp = 0;

  //---------------------------------------------------------------Part 3
  if (!commFailure) {
  if (tempSlave1.available()) {
    highTemp1 = tempSlave1.read();
    notGottenSerial1 = false;
  }
  else if (!notGottenSerial1) { // this occure when the arduino recognizes that it has not gotten serial data
    timenotGottenSerial1 = millis();
    notGottenSerial1 = true;
  }
  else {
    serial1Absence = millis() - timenotGottenSerial1;
  }
  // identical IFs
  if (tempSlave2.available()) {
    highTemp2 = tempSlave2.read();
    notGottenSerial2 = false;
  }
  else if (!notGottenSerial2) {
    timenotGottenSerial2 = millis();
    notGottenSerial2 = true;
  }
  else {
    serial2Absence = millis() - timenotGottenSerial2;
  }
  //--------------------------------------------------------------Part 4
  if ((serial1Absence > 1000) || (serial2Absence > 1000)) {
    highestTemp = 255;
  }
  highestTemp = max(highestTemp, highTemp1);
  highestTemp = max(highestTemp, highTemp2);
  Serial.write(highestTemp);
  
} // this bracket ends the "if(!commfailure) {" on line 34
}// this one ends the "if (millis() > loopCounter) {" on line 31
}
  //----------------------------------------------------------------End of Loop
  





