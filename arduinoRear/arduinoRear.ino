/*
Arduino 1
Type: Mega
Use: Relay communication between other Arduinos, activate brake light, catch emergency shutoff button presses,
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
#include <EEPROM.h>;

<<<<<<< HEAD
int digitalRelay1 = 53;
int digitalRelay2 = 51;
int digitalRelay3 = 49;
int digitalRelay4 = 47;
int digitalRelay5 = 45;
int digitalRelay6 = 41;
int digitalImd1 = 2; // HSOK pin
int digitalImd2 = 3; // PWM read pin
int digitalBrake = 31;
int digitalReady2DriveSound = 29;
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

String inputCmd1 = "";
String inputCmd2 = "";
String inputCmd3 = "";
boolean stringComplete1 = false;
boolean stringComplete2 = false;
boolean stringComplete3 = false;
unsigned long timeoutRx1;//Use Serial1 for Arduino 2 - Dash
unsigned long timeoutRx2;//Use Serial2 for Arduino 4 - Battery 1
unsigned long timeoutRx3;//Use Serial3 for Arduino 5 - Battery 2
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)
boolean eepromCheckGood = false;//Used when checking if eeprom error code is set
boolean eepromChecked = false;//Used so eeprom checked only once when starting or restarting
boolean ready2Drive = false;
int startupSequence = 0;
int eepromErrCode = 0;//Holds eeprom error code
int startupNote = 200;

void setup() {
    Serial.begin(115200);//Talk back to computer
    Serial1.begin(115200);//ar2
    Serial2.begin(115200);//ar4
    Serial3.begin(115200);//ar5
    inputCmd1.reserve(50);
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
    timeoutRx1 = 1000;
    timeoutRx2 = 1000;
    timeoutRx3 = 1000;
    runLoop = 0;
}

void loop() {
    /*************************************
    Commands that run whether or not car is ready to drive
    *************************************/
    if (stringComplete1) {//Received command from AR2
        if (inputCmd2 == "ar1:restart") {
            shutdownHard(0);
        } else if (inputCmd2.substring(0,10) == "ar1:print:") {
            Serial.print(millis());
            Serial.print(" - ");
            Serial.println(inputCmd2.substring(10));//todo make sure this works
        } else if (inputCmd2 == "ar1:brake:1") {
            digitalWrite(digitalBrake, HIGH);
        } else if (inputCmd2 == "ar1:brake:0") {
            digitalWrite(digitalBrake, LOW);
        }

    inputCmd2 = "";
    stringComplete2 = false;
    }
    /*************************************
    End commands
    *************************************/
    if (!ready2Drive) {
        if (!eepromChecked) {//Runs on startup and after high voltage shutdowns
            //Check if EEPROM error code was set
            eepromErrCode = EEPROM.read(0);
            if (eepromErrCode == 255){//todo make sure this is correct blank code
                eepromCheckGood = true;
            }
            if (eepromErrCode == 1) {
                Serial1.println("ar2:amsBmsFaultLed:1");//Make dash Arduino turn on error light
                Serial.println("BMS fault - please reset");
            }
            if (eepromErrCode == 2) {
                Serial1.println("ar2:imdFaultLed:1");//Make dash Arduino turn on error light
                Serial.println("IMD fault - please reset");
            }
            eepromChecked = true;
        }
        if (!eepromCheckGood) {//While there is an un reset eeprom error
            //todo query BMS and do not start up if there's still an error
            if (eepromErrCode == 1 && analogRead(analogBmsFaultReset) > 1000) {//If eepromerrcode == 1 (BMS) and button pressed (5 volts reading when closed)
                EEPROM.write(0,255);//Delete error from EEPROM
                eepromCheckGood = true;//Exit this loop
                Serial1.println("ar2:amsBmsFaultLed:0");//Make dash Arduino turn off error light
                Serial.println("BMS fault cleared");
            }
            if (eepromErrCode == 2 && analogRead(analogImdFaultReset) > 1000) {//If eepromerrcode == 2 (IMD) and button pressed (5 volts reading when closed)
                EEPROM.write(0,255);//Delete error from EEPROM
                eepromCheckGood = true;//Exit this loop
                Serial1.println("ar2:imdFaultLed:0");//Make dash Arduino turn off error light
                Serial.println("IMD fault cleared");
            }
        }
        if (eepromChecked && eepromCheckGood) {//Eeprom check is finished
            if (startupSequence == 0) {
                //Begin startup sequence
                Serial.println("Waiting to begin startup sequence");

                //todo wait for initialize switch to continue
                if (false) {
                    Serial.println("Startup sequence activated");
                    startupSequence = 1;
                }
            }
            if (startupSequence == 1) {
                //Close relays 1, 3
                digitalWrite(digitalRelay1, HIGH);
                digitalWrite(digitalRelay3, HIGH);
                startupSequence = 2;
                runLoop = millis() + 1000;
            }
            if (startupSequence == 2 && runLoop < millis()) {//At least 1 second has passed since sequence part 1
                //Close relays 2, precharge
                digitalWrite(digitalRelay2, HIGH);
                digitalWrite(digitalRelay5, HIGH);
                startupSequence = 3;
                runLoop = millis() + 3000;
            }
            if (startupSequence == 3 && runLoop < millis()) {
                //todo check voltage behind dcdc converter (tractive active lights)
                if (false) {
                    //todo wait for button press
                    if (false) {
                        digitalWrite(digitalRelay6, HIGH);
                        digitalWrite(digitalRelay5, LOW);
                        digitalWrite(digitalRelay4, HIGH);
                        startupSequence = 4;
                        runLoop = millis() + 2000;
                        Serial.println("Playing ready 2 drive sound");
                    }
                }
            }
            if (startupSequence == 4 && runLoop > millis()) {
                //Ready to drive sound
                tone(digitalReady2DriveSound, startupNote);
                note += 2;
            }
            if (startupSequence == 4 && runLoop < millis()) {
                Serial.println("Vehicle ready to drive");
                Serial1.println("ar2:ready2Drive");
                Serial1.println("ar3:ready2Drive");
            }
        }
    }

    else {//No eeprom errors
        if (stringComplete3) {//Received command from ar3
            //Put stuff here
            inputCmd3 = "";
            stringComplete3 = false;
        }
        if (stringComplete4) {//Received command from ar4
            //Put stuff here
            inputCmd4 = "";
            stringComplete4 = false;
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


    serialTimeout();
}

void serialTimeout() {
    if (timeoutRx1 < millis() || timeoutRx2 < millis() || timeoutRx3 < millis()) {//If 1 second has passed since receiving a complete command from all 3 connected Arduinos turn off low voltage ***SOMETHING HAS GONE WRONG***
        Serial.println(millis());
        if(timeoutRx1 < millis()) {
            Serial.println("ar1 lost connection to ar2");
        }
        if(timeoutRx2 < millis()) {
            Serial.println("ar1 lost connection to ar4");
        }
        if(timeoutRx3 < millis()) {
            Serial.println("ar1 lost connection to ar5");
        }
        shutdownHard(10);
    }
}

void serialEvent1() {//Receives commands from ar2
    while (Serial1.available()) {
        char newChar = (char)Serial1.read();
        if (newChar == '\n') {//NOTE: inputCmd1 does NOT include \n
            stringComplete1 = true;
            timeoutRx1 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            Serial.println("recvd ar2: "+inputCmd1);
        }else {
            inputCmd1 += newChar;
        }
    }
}

void serialEvent2() {//Receives commands from ar4
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {//NOTE: inputCmd2 does NOT include \n
            stringComplete2 = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            Serial.println("recvd ar4: "+inputCmd2);
        }else {
            inputCmd2 += newChar;
        }
    }
}

void serialEvent3() {//Receives commands from ar5
    while (Serial3.available()) {
        char newChar = (char)Serial3.read();
        if (newChar == '\n') {//NOTE: inputCmd3 does NOT include \n
            stringComplete3 = true;
            timeoutRx3 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            Serial.println("recvd ar5: "+inputCmd3);
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
    */
    //ALL shutdowns
    digitalWrite(digitalRelay1, LOW);
    digitalWrite(digitalRelay2, LOW);
    digitalWrite(digitalRelay3, LOW);
    digitalWrite(digitalRelay4, LOW);
    digitalWrite(digitalRelay6, LOW);//Close discharge AIR
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
    Serial1.println("ar2:restart");
    Serial1.println("ar3:restart");
    //todo ar4 & ar5 don't need to reset right?
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
