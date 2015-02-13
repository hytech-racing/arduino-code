

// arduino at dashboard
// this code is sparse in comments; check
// the document "code_explanations" for an explanation of this program
// 3 stages:  error checking, error display, and then normal functionality
//---------------------------------------------------------------------------------Part 1
#include <LiquidCrystal.h>
#include <Wire.h>
unsigned long loopCounter = 0;
const int pumpLight = 13;
const int IMDlight = 12;
const int BMSlight = 11;
const int CycleButton = 10;
const int motorSignalPin = 2;
const int accPotPin = 0;
const int secondAccPotPin = 1;
const int brakePotPin = 2;
const int rpmInputPin = 3;
int motorPulseQuantity = 0;
int displayStatus = 0;
int errorCodeReceive = 0;
int errorCodeLocal = 0;
int accPedalDifference = 0;
int pedal1Val = 0;
int pedal2Val = 0;
int brakeVal = 0;
int rpmVal = 0;

boolean carStartedYet = false;
boolean cycleButtonOn = false;
boolean oldCycleButtonOn = false;





void setup() {
  Wire.begin(5);
  Serial.begin(9600);
  Wire.onRequest(giveDataToCenter);
  Wire.onReceive(getDataFromCenter);
}
//-----------------------------------------------------------------------------------Part 2
void loop() {
  while(!carStartedYet) {
    // write to lcd that the car has not started yet
  }
  if (millis() > loopCounter) {
  loopCounter = millis() + 100;  
  
  pedal1Val = analogRead(accPotPin);
  pedal2Val = analogRead(secondAccPotPin);
  accPedalDifference = abs(pedal1Val - pedal2Val);
  if (accPedalDifference > 25) {
    errorCodeLocal = errorCodeLocal & 2; // bitwise AND
  }
  if (errorCodeReceive > 0 || errorCodeLocal > 0) {
    motorPulseQuantity = 0;
    analogWrite(motorSignalPin, motorPulseQuantity)
    // write to LCD that an error has occurred
    //turn on LEDs depending on what is wrong
  }
  else {
    brakeVal = analogRead(brakePotPin);
    pedal1Val = min(pedal1Val, pedal2Val);
    if (brakeVal < 20) {
      motorPulseQuantity = pedalVal/4; //--------------- PROPER SCALING FOR MOTOR CONTROLLER NEEDED
     analogWrite(motorSignalPin, motorPulseQuantity);
    }
    else {
      analogWrite(motorSignalPin, 0);
    }
    rpmVal = analogRead(rpmInputPin);
    // calculate MPH from rpmVal
    // write this value to the LCD screen
    


    
  }

  
  
  
    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}  
} // ------------------------------------------------------------END OF LOOP

void giveDataToCenter() {
}
void getDataFromCenter() {
  carStartedYet = true;
}
  
