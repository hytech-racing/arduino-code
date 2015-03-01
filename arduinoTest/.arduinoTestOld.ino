/*
Arduino 3
Type: Uno (currently, for testing)
Use: Currently for testing other Arduinos
*/

String inputCmd = "";
boolean stringComplete = false;
unsigned long timeoutRx;//Stores millisecond value to shut off if communication is lost
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)
int ledPinErr = 13;
int ledPinYes = 12;

void setup() {
  Serial.begin(115200);
  //Wait 1 second for communication before throwing error
  timeoutRx = 1000;
  runLoop = 0;
  pinMode(ledPinErr,OUTPUT);
  pinMode(ledPinYes,OUTPUT);
}

void loop() {
  if (stringComplete) {//Received something on serial
    if(inputCmd.substring(0,12) == "ar3:testVal:") {
      digitalWrite(ledPinYes,HIGH);
    }
    inputCmd = "";
    stringComplete = false;
  }
  if (runLoop < millis()){//Runs 10x per second
    runLoop = millis() + 100;//Push runLoop up 100 ms
    Serial.println("ar2:hi");//Send something across relay
  }
  if (timeoutRx < millis()) {//If 1 second has passed since receiving a complete command from Arduino turn off low voltage ***SOMETHING HAS GONE WRONG***
    //todo Code to shutdown
    //For now, activate LED
    digitalWrite(ledPinErr, HIGH);
  } else {
    digitalWrite(ledPinErr, LOW);
  }
}

void serialEvent() {
  while (Serial.available()) {
    char newChar = (char)Serial.read();
    if (newChar == '\n') {
      stringComplete = true;
      timeoutRx = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
    }else {
      inputCmd += newChar;
    }
  }
}
