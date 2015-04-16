/*
Arduino 1
Type: Mega
Use: Start vehicle, activate brake light, shut off vehicle
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
int digitalRelay7 = 8;
int digitalReady2DriveSound = 9; //Relay 8
int digitalImd = 32; // PWM read pin
int digitalTransistor4 = 46;
int digitalBrake = 48; //transistor3
int digitalTransistor2 = 50;
int digitalPumpFan = 52; //transistor1
int analogHSOK = 7;
int analogAmbientTemp = 8; //todo
int analogDCDCTemp = 9; //todo
int analogImdBmsFaultReset = 11;
int analogThermistor1 = 12;
int analogThermistor2 = 13;
int analogThermistor3 = 14;
int analogMotorCtrlTemp; //todo find pin number
int analogCheckShutdownButtons; //todo find pin number
int analogCheckDcDc; //todo find pin number
int Bms_SOC = 3;//todo bms pins might be mixed up
    int Bms_SOC_Val = 0;
int Bms_Amps = 4;
    int Bms_Amps_Val = 0;
    int Bms_Amps_Actual = 0;
    int Bms_Allowed_Current = 0;
int Bms_DCL = 5;
    int Bms_DCL_Val = 0;

/**Added by Nathan C to force it to compile*/
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

String inputCmdStream1 = "";
String inputCmdStream2 = "";
String inputCmdStream3 = "";
boolean stringComplete1 = false;
boolean stringComplete2 = false;
boolean stringComplete3 = false;
String inputCmd1;
String inputCmd2;
String inputCmd3;
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
int newLineIndex;

void setup() {
    Serial.begin(115200);//Talk back to computer
    //Initialize output pins
    pinMode(digitalRelay1, OUTPUT);
    pinMode(digitalRelay2, OUTPUT);
    pinMode(digitalRelay3, OUTPUT);
    pinMode(digitalRelay4, OUTPUT);
    pinMode(digitalRelay5, OUTPUT);
    pinMode(digitalRelay6, OUTPUT);
    pinMode(digitalRelay7, OUTPUT);
    pinMode(digitalReady2DriveSound, OUTPUT);
    pinMode(digitalImd, OUTPUT);//todo output or input?
    pinMode(digitalTransistor4, OUTPUT);
    pinMode(digitalBrake, OUTPUT);
    pinMode(digitalTransistor2, OUTPUT);
    pinMode(digitalPumpFan, OUTPUT);

    digitalWrite(digitalPumpFan, HIGH);//Turn on cooling fan
    runLoop = 0;
}

void loop() {

    boolean writeEeprom = false;
    if (!writeEeprom) {
      EEPROM.write(10, 255);
      writeEeprom = true;
    }
    /******************************
    Throttle reading code
    ******************************/
    //Read analog values all at once
    pot1ValAdjusted = analogRead(pot1);
    pot2ValAdjusted = analogRead(pot2);
    pot3ValAdjusted = analogRead(pot3);

    if (pot1ValAdjusted > 1000 || pot1ValAdjusted < 10 || pot2ValAdjusted > 1000 || pot2ValAdjusted < 10) {
        reset(11);
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

    if (pot3ValAdjusted > 0) { //Brake light
        digitalWrite(digitalBrake, HIGH);
    } else if (pot3ValAdjusted <= 0) {
        digitalWrite(digitalBrake, LOW);
    }

    if (potAccAdjDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
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
void reset(int i) {
    Serial.println("Oh noes!");
    Serial.println(i);
}
