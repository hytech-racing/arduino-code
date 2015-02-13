
// This code is sparse in comments; check the document "Code_Explanations" or of a similar name for an explanation.
//----------------------------------------------------------------Part 1
int sensorReadings[9] = {0, 0, 0, 0, 0, 0, 0, 0};
int variablePlaceWriteTo = 0;
int readFailure = 0;
int i = 0;
int highestTemp = 0;
boolean commFailure = false;
unsigned long loopCounter = 0;
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  //delay(100);
}
//----------------------------------------------------------------Part 2
void loop() {
  if (millis() > loopCounter) {
  loopCounter = millis() + 100; // 10 times/second  
  
  resetVars(); // defined below
  for (i = 72; i < 76; i++) {
    Wire.flush();
    Wire.requestFrom(i, 2); // check how many bytes to recieve
    while(Wire.available()) {
      sensorReadings[variablePlaceWriteTo] = Wire.read();
      variablePlaceWriteTo++;
    }
  }
  //--------------------------------------------------------------Part 3
  if (variablePlaceWriteTo != 8) {
    readFailure++;
    commFailure = true;
  }
  else {
    readFailure = 0;
  }
  // -------------------------------------------------------------Part 4
  if (!commFailure) {
    for (i = 0; i < 7; i = i+2) {
      sensorReadings[i] = sensorReadings[i]*4;
      if ((sensorReadings[i+1] & 128) == 128) { // bitwise AND
        sensorReadings[i] = sensorReadings[i]+2;  // extracts final two bits
      }
      if ((sensorReadings[i+1] & 64) == 64) {
        sensorReadings[i] = sensorReadings[i]+1;
      }
    }
    //------------------------------------------------------------Part 5
    highestTemp = max(sensorReadings[0], sensorReadings[2]);
    highestTemp = max(highestTemp, sensorReadings[4]);
    highestTemp = max(highestTemp, sensorReadings[6]);
    Serial.write(highestTemp);
                                  // check for best delay time
  }
  else if (readFailure > 5) {  // if reading the sensors has failed 5 times in a row
    Serial.write(255);
    
  }
}
}
//----------------------------------------------------------------End Of Loop     

void resetVars() {     // reset variables function
  commFailure = false;
  variablePlaceWriteTo = 0;
  for (i = 0; i < 7; i++) {
    sensorReadings[i] = 0;
  }
}
      
