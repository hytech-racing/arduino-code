/*
Arduino 1
Type: Mega
Use: Start vehicle, activate brake light, shut off vehicle
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
#include <EEPROM.h>;

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
int digitalPumpFan = 30;
int analogImdFaultReset = 0;
int analogBmsFaultReset = 1;
int analogMotorCtrlTemp = 2;
int analogCheckShutdownButtons = 9;
int analogCheckDcDc = 10;
int Bms_SOC = 11;
    int Bms_SOC_Val = 0;
int Bms_Amps = 12;
    int Bms_Amps_Val = 0;
    int Bms_Amps_Actual = 0;
    int Bms_Allowed_Current = 0;
int Bms_DCL = 13;
    int Bms_DCL_Val = 0;
int analogBms4 = 14;
int analogBms5 = 15;

/**Added by Nathan C to force it to compile*/
int analogBms1 = 0;
int analogBms2 = 0;
int analogBms3 = 0;
unsigned long highPulse;
unsigned long lowPulse;
unsigned long totalPulse;
unsigned long AmpsOutReading;

int pot1 = 0;//Pot1 and Pot2 used for acceleration
int pot2 = 1;
int pot3 = 2;//Pot3 used for brake
int pot1High = 376;//Pedal pressed
int pot1Low = 147;//Pedal resting//todo right now the low vals are when a little pressure is applied
int pot2High = 462;
int pot2Low = 226;
int pot3High = 468;//Pot3 used for brake
int pot3Low = 336;

/*************************************
END CONFIGURATION
*************************************/

float pot1ValAdjusted;
float pot2ValAdjusted;
float pot3ValAdjusted;
float potAccAdjDiff;//Holds the difference between two accelerator readings
float pot1Range = pot1High - pot1Low;//Ranges that each pot will move (used for percentage calcs)
float pot2Range = pot2High - pot2Low;
float pot3Range = pot3High - pot3Low;
float torqueVal;//0-1000 mapped value for torque
float torqueValAdjusted;//0-255 adjusted exponentially
boolean brakePlausActive = false;//Set to true if brakes actuated && torque encoder > 25%
boolean brakeLightActive = false;

String inputCmdStream1 = "";
String inputCmdStream2 = "";
String inputCmdStream3 = "";
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
int startupSequence = 0;//Holds startup stage
int eepromErrCode = 0;//Holds eeprom error code
int startupNote = 200;
int dashSwitch1Val = -1;
int dashSwitch2Val = -1;
int dashSwitch3Val = -1;
int dashSwitch4Val = -1;

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
    pinMode(digitalPumpFan, OUTPUT);
    //Initialize input pins
    pinMode(analogImdFaultReset, INPUT);
    pinMode(analogBmsFaultReset, INPUT);
    pinMode(analogMotorCtrlTemp, INPUT);
    pinMode(analogCheckShutdownButtons, INPUT);
    pinMode(analogCheckDcDc, INPUT);
    pinMode(analogBms1, INPUT);
    pinMode(analogBms2, INPUT);
    pinMode(analogBms3, INPUT);
    pinMode(analogBms4, INPUT);
    pinMode(analogBms5, INPUT);

    digitalWrite(digitalPumpFan, HIGH);//Turn on cooling fan

    //Wait 1 second for communication before throwing error
    timeoutRx1 = 1000;
    timeoutRx2 = 1000;
    timeoutRx3 = 1000;
    runLoop = 0;
}

void loop() {
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
                if (dashSwitch1Val != 1) {//Bypass switch is not neutral
                    Serial1.println("ar2:initSwitchFault:1");
                    Serial.println("Init switch in improper position");
                }
                if (dashSwitch1Val == 1 && dashSwitch2Val == 0) {//Bypass switch is neutral and init button not pressed
                    Serial1.println("ar2:initSwitchFault:0");//Make dash Arduino turn off error light
                    Serial.println("Init switch position fault cleared");
                    startupSequence = 1;
                }
            }
            if (startupSequence == 1) {
                //Begin startup sequence
                Serial.println("Waiting to begin startup sequence");

                if (dashSwitch2Val == 1) {//wait for initialize switch to continue
                    Serial.println("Startup sequence activated");
                    startupSequence = 2;
                }
            }
            if (startupSequence == 2) {
                //Close relays 1, 3
                digitalWrite(digitalRelay1, HIGH);
                digitalWrite(digitalRelay3, HIGH);
                startupSequence = 3;
                runLoop = millis() + 1000;
            }
            if (startupSequence == 3 && runLoop < millis()) {//At least 1 second has passed since sequence part 1
                //Close relays 2, precharge
                digitalWrite(digitalRelay2, HIGH);
                digitalWrite(digitalRelay5, HIGH);
                startupSequence = 4;
                runLoop = millis() + 3000;
            }
            if (startupSequence == 4 && runLoop < millis()) {
                //todo check voltage behind dcdc converter (tractive active lights)
                if (false) {
                    boolean startupVoltageLedOn = false;
                    if (!startupVoltageLedOn) {
                        Serial1.println("ar2:startupLed3:1");//todo maybe change which LED
                    }
                    if (dashSwitch2Val == 1) {//wait for button press
                        Serial1.println("ar2:startupLed3:0");
                        digitalWrite(digitalRelay6, HIGH);
                        digitalWrite(digitalRelay5, LOW);
                        digitalWrite(digitalRelay4, HIGH);
                        startupSequence = 5;
                    }
                }
            }
            if (startupSequence == 5) {
                if (dashSwitch1Val == 0) {//wait till init switch is down
                    startupSequence = 6;
                }
            }
            if (startupSequence == 6) {
                if (dashSwitch2Val == 1) {//wait till init button is pressed again
                    runLoop = millis() + 2000;
                    Serial.println("Playing ready 2 drive sound");
                    startupSequence = 7;
                }
            }
            if (startupSequence == 7 && runLoop > millis()) {
                //Ready to drive sound
                tone(digitalReady2DriveSound, startupNote);
                startupNote += 2;
            }
            if (startupSequence == 7 && runLoop < millis()) {
                Serial.println("Vehicle ready to drive");
                Serial1.println("ar2:ready2Drive");
                Serial1.println("ar3:ready2Drive");
                //todo tell motor controller ready 2 drive
                ready2Drive = true;
            }
        }
    }

    else {//No eeprom errors

    }

    /*************************************
    Commands that run whether or not car is ready to drive
    *************************************/
    if (stringComplete1) {//Received command from AR2
        int newLineIndex = inputCmdStream1.indexOf('/n');
        if (newLineIndex > -1) {
            String inputCmd1 = inputCmdStream1.substring(0,newLineIndex);
            inputCmdStream1 = inputCmdStream1.substring(newLineIndex + 1);
        } else {
            String inputCmd1 = inputCmdStream1;
            stringComplete1 = false;
        }
        if (inputCmd1 == "ar1:restart") {
            reset(0);
        } else if (inputCmd1.substring(0,10) == "ar1:print:") {
            Serial.print(millis());
            Serial.print(" - ");
            Serial.println(inputCmd1.substring(10));//todo make sure this works
        } else if (inputCmd1.substring(0,17) == "ar1:dashSwitches:") {
            char switchRead = inputCmd1.charAt(17);
            dashSwitch1Val = switchRead - '0';//todo make sure char to int conversion works
            char switchRead = inputCmd1.charAt(18);
            dashSwitch2Val = switchRead - '0';
            char switchRead = inputCmd1.charAt(19);
            dashSwitch3Val = switchRead - '0';
            char switchRead = inputCmd1.charAt(20);
            dashSwitch4Val = switchRead - '0';
        }else if (inputCmd1 == "ar1:brake:1") {
            digitalWrite(digitalBrake, HIGH);
        } else if (inputCmd1 == "ar1:brake:0") {
            digitalWrite(digitalBrake, LOW);
        }
    }

    if (stringComplete2) {//Received command from ar4
        int newLineIndex = inputCmdStream2.indexOf('/n');
        if (newLineIndex > -1) {
            String inputCmd2 = inputCmdStream2.substring(0,newLineIndex);
            inputCmdStream2 = inputCmdStream2.substring(newLineIndex + 1);
        } else {
            String inputCmd1 = inputCmdStream1;
            stringComplete1 = false;
        }
        //Put stuff here
    }
    if (stringComplete3) {//Received command from ar5
        int newLineIndex = inputCmdStream3.indexOf('/n');
        if (newLineIndex > -1) {
            String inputCmd3 = inputCmdStream3.substring(0,newLineIndex);
            inputCmdStream3 = inputCmdStream3.substring(newLineIndex + 1);
        } else {
            String inputCmd1 = inputCmdStream1;
            stringComplete1 = false;
        }
        //Put stuff here
    }
    if (runLoop < millis()) {//Runs 10x per second
        runLoop = millis() + 100;//Push runLoop up 100 ms
        Serial2.println("ar2:hi");
        Serial3.println("ar3:hi");

        //todo read motor/controller/water temp - pins A2-A3
        //todo write error code if emergency buttons pressed? - pins A6-A10
        //todo SOMETHING with bms????? - pins A11-A14
        //todo read BMS charge "charge/discharge enable" ground conn - pin A15

        //todo BMS shutdown
        reset(2);
        ////////////////////////////////////////////////////////////////////// READ IMD
        if (digitalRead(digitalImd1) == HIGH){
            highPulse = pulseIn(7, HIGH, 1500000);
            lowPulse = pulseIn(7, LOW, 1500000);
            totalPulse = highPulse + lowPulse;
            if (totalPulse > 200000) {
               reset(1); // BMS detects a fault
            }
            else {
                reset(4); // BMS gets disconnected from high voltage system
            }
        }
        ///////////////////////////////////////////////////////////////////// READ BMS
        Bms_SOC_Val = analogRead(Bms_SOC);
        Bms_Amps_Val = analogRead(Bms_Amps);
        Bms_DCL_Val = analogRead(Bms_DCL);
        Bms_Amps_Actual = (map(AmpsOutReading, 0, 1024, 0, 1250)-(1250/2)); // NEED TO USE CHANNEL 2 OF CURRENT SENSOR
        Bms_Allowed_Current = map(Bms_DCL_Val, 0, 1024, 0, 200); // CHANGE 200 TO OUR MAX ALLOWED CURRENT
        if (Bms_SOC_Val < 102) { // less than 10% of battery left
            reset(5);
        }
        else if (Bms_Amps_Actual > Bms_Allowed_Current) {
            reset(12);
        }

        /******************************
        Throttle reading code
        ******************************/
        //Read analog values all at once
        pot1ValAdjusted = analogRead(pot1);
        pot2ValAdjusted = analogRead(pot2);
        pot3ValAdjusted = analogRead(pot3);

        if (pot1ValAdjusted > 1000 || pot1ValAdjusted < 10 || pot2ValAdjusted > 1000 || pot2ValAdjusted < 10) {
            reset(0);
            Serial.println("Throttle encoder short detected");
        }

        //Now calculate remapped values
        pot1ValAdjusted = pot1ValAdjusted - pot1Low;
        pot1ValAdjusted = pot1ValAdjusted * 1000;
        pot1ValAdjusted = pot1ValAdjusted / pot1Range;//new mapped value from 0-1000

        pot2ValAdjusted = pot2ValAdjusted - pot2Low;
        pot2ValAdjusted = pot2ValAdjusted * 1000;
        pot2ValAdjusted = pot2ValAdjusted / pot2Range;

        pot3ValAdjusted = pot3ValAdjusted - pot3Low;
        pot3ValAdjusted = pot3ValAdjusted * 1000;
        pot3ValAdjusted = pot3ValAdjusted / pot3Range;

        potAccAdjDiff = abs(pot1ValAdjusted-pot2ValAdjusted);//Get difference between torque sensors
        if (pot2ValAdjusted > pot1ValAdjusted) {//Torque is lowest of two torque sensors
            torqueVal = pot1ValAdjusted;
        } else {
            torqueVal = pot2ValAdjusted;
        }
        if (torqueVal < 0) {
            torqueValAdjusted = 0;
        } else {
            torqueValAdjusted = 255 * pow((torqueVal/1000), 2);
        }
        if (torqueValAdjusted > 255) {
            torqueValAdjusted = 255;
        }
        Serial.print("Throttle value ");
        Serial.println(torqueValAdjusted);//Prints torque value to computer

        if (pot3ValAdjusted > 0 && !brakeLightActive) { //Brake light
            Serial1.println("ar1:brake:1");
            Serial.println("Brake lights on");
        } else if (pot3ValAdjusted <= 0 && brakeLightActive) {
            Serial1.println("ar1:brake:0");
            Serial.println("Brake lights off");
        }

        if (potAccAdjDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
            //todo error checking which can detect open circuit, short to ground and short to sensor power
            //todo does this need to shut down car or just send 0 torque val?
            //todo put error on screen
            Serial.println("Acceleration Implausibility on");
        } else {
            //todo remove error from screen
            Serial.println("Acceleration Implausibility off");
            if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
                brakePlausActive = true;
                //todo put error on screen
                Serial.println("Brake plausibility on");
            } else if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                brakePlausActive = false;
                //todo remove error from screen
                Serial.println("Brake plausibility off");
            }
            if (brakePlausActive) {//If brake plausibility is active
                //todo put 0 throttle on screen
            } else {//If brake plausibility is not active
                //todo put throttle on screen
            }
        }
        /******************************
        End throttle reading code
        ******************************/

        //todo Anything else this Arduino needs to do other than process received data
    }

    /*************************************
    End always-run commands
    *************************************/


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
        reset(10);
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
            inputCmdStream1 += newChar;
        }
    }
}

void serialEvent2() {//Receives commands from ar4. todo: might have problems if receives while using inputCmdStream in arduino loop
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {//NOTE: inputCmd2 does NOT include \n
            stringComplete2 = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            Serial.println("recvd ar4: "+inputCmd2);
        }else {
            inputCmdStream2 += newChar;
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
            inputCmdStream3 += newChar;
        }
    }
}

void reset(int errCode) {
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
    reset();
    //BMS shutdown (a work in progress)
    if (errCode == 1) {
        EEPROM.write(0,1);
    }
    if (errCode == 2) {
        EEPROM.write(0,2);
    }
    Serial.print(millis());
    Serial.print(" - Shutting down - ");
    Serial.println(errCode);//Now send to computer
    Serial1.println("ar2:restart");
    //todo ar4 & ar5 don't need to reset right?
    ready2Drive = false;
    startupSequence = 0;//Reset the startup sequence
    eepromChecked = false;
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
