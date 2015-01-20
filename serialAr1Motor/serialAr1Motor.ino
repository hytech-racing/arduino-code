/*
Arduino 1
Type: Mega
Use: Talk to Motor Controller and relay communication between 2 other Arduinos
*/

int ledPinGrn = 4;
int ledPinErr = 13;

String inputCmd2 = "";
String inputCmd3 = "";
boolean stringComplete2 = false;
boolean stringComplete3 = false;
unsigned long timeoutRx2;//Use Serial2 for Arduino 2 - Dash
unsigned long timeoutRx3;//Use Serial3 for Arduino 3 - Battery
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)


void setup() {
  Serial.begin(115200);//Talk back to computer
  Serial2.begin(115200);
  Serial3.begin(115200);
  inputCmd2.reserve(50);
  inputCmd3.reserve(50);
  
  pinMode(ledPinGrn, OUTPUT);
  pinMode(ledPinErr, OUTPUT);
  
  //Wait 1 second for communication before throwing error
  timeoutRx2 = 1000;
  timeoutRx3 = 1000;
  runLoop = 0;
}


void loop() {
  if (stringComplete2) {//Received command from AR2
    if (inputCmd2.substring(0,3) == "ar3") {//Is this command meant for Arduino 3?
      Serial3.println(inputCmd2);//todo this is untested
      Serial.println("Relayed from ar2 to ar3: "+inputCmd2);
    }
    else if (inputCmd2.substring(0,12) == "ar1:led:grn:") {//Is this a command to AR1 LED GRN?
      String subCmd = inputCmd2.substring(12,13);
      if (subCmd == "1") {//1 for on
        digitalWrite(ledPinGrn, HIGH);
      }else if (subCmd == "0") {//2 for off
        digitalWrite(ledPinGrn, LOW);
      }
    }
    inputCmd2 = "";
    stringComplete2 = false;
  }
  if (stringComplete3) {//Received command from AR3
    if (inputCmd3.substring(0,3) == "ar2") {//Is this command meant for Arduino 2?
      Serial2.println(inputCmd3);//todo this is untested
      Serial.println("Relayed from ar3 to ar2: "+inputCmd3);
    }
    inputCmd3 = "";
    stringComplete3 = false;
  }
  if (runLoop < millis()) {//Runs 10x per second
    runLoop = millis() + 100;//Push runLoop up 100 ms
    //todo Anything this Arduino needs to do other than process received data
  }
  if (timeoutRx2 < millis() || timeoutRx3 < millis()) {//If 1 second has passed since receiving a complete command from both Arduinos turn off low voltage ***SOMETHING HAS GONE WRONG***
    //todo Code to shutdown
    //For now, activate LED and print error
    Serial.println(millis());
    if(timeoutRx2 < millis()) {
      Serial.println("ar1 lost connection to ar2");
    }
    if(timeoutRx3 < millis()) {
      Serial.println("ar1 lost connection to ar3");
    }
    digitalWrite(ledPinErr, HIGH);
  } else {
    digitalWrite(ledPinErr, LOW);
  }
}

void serialEvent2() {//Receive bytes from AR2
  while (Serial2.available()) {
    char newChar = (char)Serial2.read();
    if (newChar == '\n') {//NOTE: inputCmd2 does NOT include \n
      stringComplete2 = true;
      timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
      Serial.println("recvd ar2: "+inputCmd2);
    }else {
      inputCmd2 += newChar;
    }
  }
}

void serialEvent3() {//Receive bytes from AR3
  while (Serial3.available()) {
    char newChar = (char)Serial3.read();
    if (newChar == '\n') {//NOTE: inputCmd3 does NOT include \n
      stringComplete3 = true;
      timeoutRx3 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
      Serial.println("recvd ar3: "+inputCmd3);
    }else {
      inputCmd3 += newChar;
    }
  }
}
