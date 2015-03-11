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
int digitalImd1 = 9; // HSOK pin
int digitalImd2 = 10; // PWM read pin
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
int Bms_SOC = 11;
    int Bms_SOC_Val = 0;
int Bms_Amps = 12;
    int Bms_Amps_Val = 0;
    Bms_Amps_Actual = 0;
    Bms_Allowed_Current = 0;
int Bms_DCL = 13;
    int Bms_DCL_Val = 0;
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
boolean eepromCheckGood = false;//Used when checking if eeprom error code is set, triggers startup function
serialErrorTxt1 = "Shutoff error code (must reset): ";

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

    //Wait 1 second for communication before throwing error
    timeoutRx2 = 1000;
    timeoutRx3 = 1000;
    runLoop = 0;
}

void loop() {
    if (!eepromCheckGood) {//Runs on startup and after high voltage shutdowns
        unsigned long startupLoop = 0;//Stores ms value to run startup loop every so often
        //Check if EEPROM error code was set
        int eepromErrCode = EEPROM.read(0);

        if (eepromErrCode == 255){//todo make sure this is correct blank code
            eepromCheckGood = true;
        }
        if (eepromErrCode == 1) {
            Serial2.println("ar2:amsBmsFaultLed:1");//Make dash Arduino turn on error light
        }
        if (eepromErrCode == 2) {
            Serial2.println("ar2:imdFaultLed:1");//Make dash Arduino turn on error light
        }
        while (!eepromCheckGood) {//While there is an un reset eeprom error
            //todo query BMS and do not start up if there's still an error
            if (eepromErrCode == 1 && analogRead(analogBmsFaultReset) > 1000) {//If eepromerrcode == 1 (BMS) and button pressed (5 volts reading when closed)
                EEPROM.write(0,255);//Delete error from EEPROM
                eepromCheckGood = true;//Exit this loop
                Serial2.println("ar2:amsBmsFaultLed:0");//Make dash Arduino turn off error light
            }
            if (eepromErrCode == 2 && analogRead(analogImdFaultReset) > 1000) {//If eepromerrcode == 2 (IMD) and button pressed (5 volts reading when closed)
                EEPROM.write(0,255);//Delete error from EEPROM
                eepromCheckGood = true;//Exit this loop
                Serial2.println("ar2:imdFaultLed:0");//Make dash Arduino turn off error light
            }
            if (startupLoop < millis()) {
                startupLoop = millis() + 500;//Run every .5 seconds
                Serial.println(serialErrorTxt1 + eepromErrCode);//Send to computer
                Serial2.println("ar2:waitErr");
                Serial3.println("ar3:waitErr");
            }
        }

        //todo wait for initialize switch to activate relays 1-3
        //Activate relay 4 (precharge) for at least 3 seconds
        digitalWrite(digitalRelay4, HIGH);
        unsigned long precharge = millis() + 3000;//Keep precharge on for 3 seconds
        while (precharge > millis()) {
            if (startupLoop < millis()) {
                startupLoop = millis() + 500;//Run every .5 seconds
                Serial.println("Precharge relay activated");
                Serial2.println("ar2:waitPrecharge");
                Serial3.println("ar3:waitPrecharge");
            }
        }
        //todo then wait for activation switch (maybe)
        //Activate relay 5 (AIR 4) and relay 6 and disable precharge immediately
        digitalWrite(digitalRelay5, HIGH);
        digitalWrite(digitalRelay6, HIGH);
        digitalWrite(digitalRelay4, LOW);
        //Ready to drive sound
        unsigned long ready2DriveSound = millis() + 2000;//Stores ms value to stop ready to drive sound after 2 seconds
        int note = 290;
        startupLoop = millis();
        while (ready2DriveSound > millis()) {//Keep drive sound on for 2 seconds but cont. sending serial comm.
            if (startupLoop < millis()) {
                startupLoop = millis() + 500;//Run every .5 seconds
                note += 150;
                tone(digitalReady2DriveSound, note, 500);
                Serial.println("Playing ready 2 drive sound");
                Serial2.println("ar2:waitR2D");
                Serial3.println("ar3:waitR2D");
            }
        }
        Serial2.println("ar2:ready2Drive");
        Serial3.println("ar3:ready2Drive");
    }
    else {//No eeprom errors
        if (stringComplete2) {//Received command from AR2
            if (inputCmd2.substring(0,3) == "ar3") {//Is this command meant for Arduino 3?
                Serial3.println(inputCmd2);
                Serial.println("Relayed from ar2 to ar3: "+inputCmd2);
            } else if (inputCmd2 == "ar1:restart") {
                shutdownHard(0);
            } else if (inputCmd2 == "ar1:brake:1") {
                digitalWrite(digitalBrake, HIGH);
            } else if (inputCmd2 == "ar1:brake:0") {
                digitalWrite(digitalBrake, LOW);
            }

        inputCmd2 = "";
        stringComplete2 = false;
        }
        if (stringComplete3) {//Received command from AR3
            if (inputCmd3.substring(0,3) == "ar2") {//If command meant for Arduino 2
                Serial2.println(inputCmd3);//todo this is untested
                Serial.println("Relayed from ar3 to ar2: "+inputCmd3);
            }
            inputCmd3 = "";
            stringComplete3 = false;
        }
        if (runLoop < millis()) {//Runs 10x per second
            runLoop = millis() + 100;//Push runLoop up 100 ms
            Serial2.println("ar2:hi");
            Serial3.println("ar3:hi");

            //todo read motor/controller/water temp - pins A2-A3
            //todo make sure water pump is on - pin A5?
            //todo write error code if emergency buttons pressed? - pins A6-A10
            //todo SOMETHING with bms????? - pins A11-A14
            //todo read BMS charge "charge/discharge enable" ground conn - pin A15

            //todo BMS shutdown
            shutdownHard(2);
            ////////////////////////////////////////////////////////////////////// READ IMD
            if (digitalRead(digitalImd1) == HIGH){
                highPulse = pulseIn(7, HIGH, 1500000);
                lowPulse = pulseIn(7, LOW, 1500000);
                totalPulse = highPulse + lowPulse;
                if (totalPulse > 200000) {
                   shutdownHard(1); // BMS detects a fault
                }
                else {
                    shutdownHard(4); // BMS gets disconnected from high voltage system
                }
            }
            ///////////////////////////////////////////////////////////////////// READ BMS
            Bms_SOC_Val = analogRead(Bms_SOC);
            Bms_Amps_Val = analogRead(Bms_Amps);
            Bms_DCL_Val = analogRead(Bms_DCL);
            Bms_Amps_Actual = (map(AmpsOutReading, 0, 1024, 0, 1250)-(1250/2)); // NEED TO USE CHANNEL 2 OF CURRENT SENSOR
            Bms_Allowed_Current = map(Bms_DCL_Val, 0, 1024, 0, 200) // CHANGE 200 TO OUR MAX ALLOWED CURRENT
            if (Bms_SOC_Val < 102) { // less than 10% of battery left
                shutdownHard(5);
            }
            else if (Bms_Amps_Actual > Bms_Allowed_Current) {
                shutdownHard(12);
            }

            //todo Anything this Arduino needs to do other than process received data
        }
    }
    if (timeoutRx2 < millis() || timeoutRx3 < millis()) {//If 1 second has passed since receiving a complete command from both Arduinos turn off low voltage ***SOMETHING HAS GONE WRONG***
        Serial.println(millis());
        if(timeoutRx2 < millis()) {
            Serial.println("ar1 lost connection to ar2");
        }
        if(timeoutRx3 < millis()) {
            Serial.println("ar1 lost connection to ar3");
        }
        shutdownHard(10);
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
    1. BMS loss of 
    2. IMD loss of insulation
    3. Cockpit ESB
    4. IMD disconnect from tractive system 
    5. Battery less than 10% capacity
    
    Resetable error codes:
    10. Lost communication
    11. Acceleration implausibility
    12. Too much power (>=5kW)
    */
    //ALL shutdowns
    digitalWrite(digitalRelay1, LOW);
    digitalWrite(digitalRelay2, LOW);
    digitalWrite(digitalRelay3, LOW);
    digitalWrite(digitalRelay4, LOW);//todo which relays do I turn off?
    //BMS shutdown (a work in progress)
    if (errCode == 1) {
        EEPROM.write(0,1);
    }
    if (errCode == 2) {
        EEPROM.write(0,2);
    }
    if (errCode == 3) {
        if (true) {
            //todo if init switch in neutral position then throw no error
        } else {
            EEPROM.write(0,3);
        }
    }
    Serial.println(errCode);//Now send to computer
    Serial2.println("ar2:restart");
    Serial3.println("ar3:restart");
    eepromCheckGood = false;
}

/*ALL SHUTDOWNS
BMS shutdown:
Cut all relays
Then eeprom error code
Error code 2 make sure switch is not in IMD bypass position
Then BMS reset switch must be pressed
Then (without having to restart) do startup checks
Check if BMS is still throwing error code (don't start high voltage if it is)

IMD reset:
Interpret error from pwm
Cut all relays
Write eeprom error code

Regular shutdown:
See from voltage divider that cockpit ESB is pressed
Cut all relays
if initialize switch is in neutral position then no error
*/

boolean queryBmsError() {//Returns true if error
  //todo talk to BMS
  return true;
}
