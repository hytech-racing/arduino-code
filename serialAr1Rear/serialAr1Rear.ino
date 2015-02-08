/*
Arduino 1
Type: Mega
Use: Relay communication between other Arduinos, activate brake light, catch emergency shutoff button presses,
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
#include <EEPROM.h>;

int digitalRelay1 = 2;
int digitalRelay2 = 3;
int digitalRelay3 = 4;
int digitalRelay4 = 5;
int digitalRelay5 = 6;
int digitalRelay6 = 7;
int digitalImd1 = 9;
int digitalImd2 = 10;
int digitalLedErr = 13;//We might not need this
int digitalBrake = 29;
int digitalReady2DriveSound = 31;
int analogImdFaultReset = 0;
int analogBmsFaultReset = 1;
int analogMotorCtrlTemp = 2;
int analogWaterTemp = 3;
int analogPumpStatus = 5;
int analogCheckInertia = 6;
int analogCheckCockpit = 7;
int analogCheckBots = 8;
int analogCheckTsms = 9;
int analogCheckDcDc = 10;
int analogBms1 = 11;
int analogBms2 = 12;
int analogBms3 = 13;
int analogBms4 = 14;
int analogBms5 = 15;

/*************************************
END CONFIGURATION
*************************************/

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
    //Initialize output pins
    pinMode(digitalRelay1, OUTPUT);
    pinMode(digitalRelay2, OUTPUT);
    pinMode(digitalRelay3, OUTPUT);
    pinMode(digitalRelay4, OUTPUT);
    pinMode(digitalRelay5, OUTPUT);
    pinMode(digitalRelay6, OUTPUT);
    pinMode(digitalImd1, OUTPUT);
    pinMode(digitalImd2, OUTPUT);
    pinMode(digitalLedErr, OUTPUT);
    pinMode(digitalBrake, OUTPUT);
    pinMode(digitalReady2DriveSound, OUTPUT);
    //Initialize input pins
    pinMode(analogImdFaultReset, INPUT);
    pinMode(analogBmsFaultReset, INPUT);
    pinMode(analogMotorCtrlTemp, INPUT);
    pinMode(analogWaterTemp, INPUT);
    pinMode(analogPumpStatus, INPUT);
    pinMode(analogCheckInertia, INPUT);
    pinMode(analogCheckCockpit, INPUT);
    pinMode(analogCheckBots, INPUT);
    pinMode(analogCheckTsms, INPUT);
    pinMode(analogCheckDcDc, INPUT);
    pinMode(analogBms1, INPUT);
    pinMode(analogBms2, INPUT);
    pinMode(analogBms3, INPUT);
    pinMode(analogBms4, INPUT);
    pinMode(analogBms5, INPUT);

    //Init these here because only needed in scope of setup()
    unsigned long startupLoop;//Stores ms value to run startup loop every so often
    unsigned long ready2DriveSound;//Stores ms value to stop ready to drive sound

    //Wait 1 second for communication before throwing error
    timeoutRx2 = 1000;
    timeoutRx3 = 1000;
    runLoop = 0;
    startupLoop = 0;

    bool ready2Startup;
    //Check if EEPROM error code was set
    byte eepromErrCode = EEPROM.read(0);

    if (eepromErrCode == 255){//todo make sure this is correct blank code
        ready2Startup = true;
    }
    while (!ready2Startup) {
        //todo if eepromerrcode == 1 (BMS) and button pressed
        if (eepromErrCode == 1 && true) {
            EEPROM.write(0,255);
            ready2Startup = true;
        }
        //todo if eepromerrcode == 2 (IMD) and button pressed (5 volts reading when closed)
        if (eepromErrCode == 2 && true) {
            EEPROM.write(0,255);
            ready2Startup = true;
        }
        if (startupLoop < millis()) {
            startupLoop = millis() + 500;//Run every .5 seconds
            Serial.println("Shutoff error code (must reset): " + eepromErrCode);//Send to computer
            Serial1.println("ar3:waitErr");//So ar1 and ar3 are receiving something on serial
        }
    }
    //todo anything else before ready to drive
    //Ready to drive
    digitalWrite(digitalReadyToDriveSound, HIGH);
    ready2DriveSound = millis() + 2000;//Stop after 2 seconds
    startupLoop = millis();
    while (ready2DriveSound > millis()) {//Keep drive sound on for 2 seconds but cont. sending serial comm.
        if (startupLoop < millis()) {
            startupLoop = millis() + 500;//Run every .5 seconds
            Serial.println("Playing ready 2 drive sound");
            Serial1.println("ar3:waitR2D");//So ar1 and ar3 are receiving something on serial
        }
    }
    digitalWrite(digitalReadyToDriveSound, LOW);
}


void loop() {
    if (stringComplete2) {//Received command from AR2
        if (inputCmd2.substring(0,3) == "ar3") {//Is this command meant for Arduino 3?
            Serial3.println(inputCmd2);//todo this is untested
            Serial.println("Relayed from ar2 to ar3: "+inputCmd2);
        }
    else if (inputCmd2.substring(0,12) == "ar1:led:brake:") {//Is this a command to AR1 LED BRAKE?
        String subCmd = inputCmd2.substring(14,15);
        if (subCmd == "1") {//1 for on
            digitalWrite(digitalBrake, HIGH);
        }else if (subCmd == "0") {//0 for off
            digitalWrite(digitalBrake, LOW);
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

        //todo read motor/controller/water temp - pins A2-A3
        //todo make sure water pump is on - pin A5?
        //todo write error code if emergency buttons pressed? - pins A6-A10
        //todo SOMETHING with bms????? - pins A11-A14
        //todo read BMS charge "charge/discharge enable" ground conn - pin A15


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
        digitalWrite(digitalLedErr, HIGH);
    } else {
        digitalWrite(digitalLedErr, LOW);
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
    } else {
      inputCmd3 += newChar;
    }
  }
}

void shutdownHard(int errCode) {
    /*
    Error codes:
    1. BMS
    2. IMD
    */
    if(errCode){
        EEPROM.write(0,(char)errCode);
    }
    //todo activate relay to shutdown
}
