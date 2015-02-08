
// This code is sparse in comments; check the document "Code_Explanations" or of a similar name for an explanation.
//----------------------------------------------------------------Part 1
int sensorReadings[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int variablePlaceWriteTo = 0;
int readFailure = 0;
int i = 0;
int highestTemp = 0;
int highTemp1 = 0;
int highTemp2 = 0;
boolean commFailure = false;
unsigned long timenotGottenSerial1 = 0;
unsigned long timenotGottenSerial2 = 0 ;
int serial1Absence = 0;
int serial2Absence = 0;
boolean notGottenSerial1 = false;
boolean notGottenSerial2 = false;
#include <Wire.h>
#include <SoftwareSerial.h>
SoftwareSerial tempSlave1(8, 9); // RX, TX
SoftwareSerial tempSlave2(10, 11); // RX, TX

void setup() {
  Wire.begin();
  Serial.begin(9600);
}
//----------------------------------------------------------------Part 2
void loop() {
  resetVars();
  exact_same_as_temp_sense_slave() ; // function defined below
  //---------------------------------------------------------------Part 3
  if (!commFailure) {
  if (tempSlave1.available()) {
    highTemp1 = tempSlave1.read();
    notGottenSerial1 = false;
  }
  else if (!notGottenSerial1) {
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
  delay(150);
} // this bracket ends the "if(!commfailure) {" on line 32
}
  //----------------------------------------------------------------End of Loop
  

void resetVars() {
  highestTemp = 0;

  commFailure = false;
  variablePlaceWriteTo = 0;
}


void exact_same_as_temp_sense_slave() {
  for (i = 72; i < 78; i++) {
    Wire.flush();
    Wire.requestFrom(i, 2); // check how many bytes ro recieve
    while(Wire.available()) {
      sensorReadings[variablePlaceWriteTo] = Wire.read();
      variablePlaceWriteTo++;
    }
  }
  if (variablePlaceWriteTo != 12) {
    readFailure++;
    commFailure = true;
  }
  else {
    readFailure = 0;
  }
  if (!commFailure) {
    for (i = 0; i < 11; i = i+2) {
      sensorReadings[i] = sensorReadings[i]*4;
      if (sensorReadings[i+1] >= 128) {
        sensorReadings[i] = sensorReadings[i]+2;
        sensorReadings[i+1] = sensorReadings[i+1] - 128;  // extracts final two bits
      }
      if (sensorReadings[i+1] >= 64) {
        sensorReadings[i] = sensorReadings[i]+1;
      }
    }
    highestTemp = max(sensorReadings[0], sensorReadings[2]);
    highestTemp = max(highestTemp, sensorReadings[4]);
    highestTemp = max(highestTemp, sensorReadings[6]);
    highestTemp = max(highestTemp, sensorReadings[8]);
    highestTemp = max(highestTemp, sensorReadings[10]);
  }
  else if (readFailure > 5) {
    highestTemp = 255;
    commFailure = false;
  } 
}
