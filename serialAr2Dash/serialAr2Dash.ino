/*
Arduino 2
Type: Uno (currently, for testing)
Use: Read and process pedal values, show values on LCD
*/

//Extended pot is low resistance
//todo set these ports to which ports are being used
//Pot1 and Pot2 used for acceleration
int pot1 = 0;
int pot2 = 1;
//Pot3 used for brake
int pot3 = 2;

//todo change these values to actual values once installed
/*
It seems simpler to find the Arduino mapped values instead of converting into mV
every time the loop runs
*/
//Pot1 and Pot2 used for acceleration
int pot1High = 0;
int pot1Low = 0;
int pot2High = 0;
int pot2Low = 0;
//Pot3 used for brake
int pot3High = 0;
int pot3Low = 0;
//Value when hard braking
int pot3Hard = 0;

String inputCmd = "";
boolean stringComplete = false;
unsigned long timeoutRx;//Stores millisecond value to shut off if communication is lost
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

float bmsPowerValue = 0;//Receives to use with brake plausibility check

char floatBuffer[32];//used to convert string to float
int pot1Val = 0;
int pot2Val = 0;
int pot3Val = 0;
int ledPinBrake = 12;
int ledPinErr = 13;

boolean regenActive = false;

void setup() {
  Serial.begin(115200);
  inputCmd.reserve(50);
  pinMode(ledPinBrake,OUTPUT);//Used for brake light
  pinMode(ledPinErr,OUTPUT);
  //Wait 1 second for communication before throwing error
  timeoutRx = 1000;
  runLoop = 0;
}

void loop() {
  if (stringComplete) {//Recieved something on serial
    if(inputCmd.substring(0,13) == "ar2:bmsPower:"){
      inputCmd.substring(13,inputCmd.length()-1).toCharArray(floatBuffer,sizeof(floatBuffer));
      bmsPowerValue = atof(floatBuffer);
    }
    inputCmd = "";
    stringComplete = false;
  }
  if(runLoop < millis()){//Runs 10x per second
    runLoop = millis() + 100;//Push runLoop up 100 ms
    pot1Val = analogRead(pot1);
    pot2Val = analogRead(pot2);
    pot3Val = analogRead(pot3);
    if(pot3Val < pot3High) { //Brake light
      digitalWrite(ledPinBrake, HIGH);
    }else {
      digitalWrite(ledPinBrake, LOW);
    }
    if(true){//todo brake plausibility check
      //If brake is pressed hard and BMS says 5kW of power (BMS arduino needs to send that value to this arduino)
      if(bmsPowerValue >= 5){//todo might not be receiving value in kW units

      }
    }
    //todo brake over travel
    if(false && regenActive == false){//Regeneration //todo what activates this?
      regenActive = true;
      Serial.println("ar1:regen:1");
    }else if(regenActive==true){
      regenActive = false;
      Serial.println("ar1:regen:0");
    }
    //todo acceleration error check (compare the two values)
    //todo send acceleration value to ar1
    String testStr = "ar3:testVal:";
    Serial.println(testStr + pot1Val); //For testing
  }
  if (timeoutRx < millis()) {//If 1 second has passed since receiving a complete command ***SOMETHING HAS GONE WRONG***
    //todo Code to shutdown
    digitalWrite(ledPinErr,HIGH);
  }else {
    digitalWrite(ledPinErr,LOW);
  }
}

void serialEvent() {
  while (Serial.available()) {
    char newChar = (char)Serial.read();
    if (newChar == '\n') {//NOTE: inputCmd does NOT include \n
      stringComplete = true;
      timeoutRx = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
      }else {
        inputCmd += newChar;
      }
    }
  }
