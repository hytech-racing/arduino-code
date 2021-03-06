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
int digitalRelay5 = 6; //Ready to drive sound
int digitalRelay6 = 7; //Throttle control
int digitalRelay7 = 8; //Discharge
int digitalRelay8 = 9; //Precharge
int digitalImd = 32; // PWM read pin
int digitalTransistor4 = 46;
int digitalBrake = 48; //transistor3
int digitalTransistor2 = 50;
int digitalPumpFan = 52; //transistor1
int mux8 = 40;
int mux4 = 28;
int mux2 = 36;
int mux1 = 34;
int analogHSOK = 7;
int analogAmbientTemp = 8; //todo
int analogDCDCTemp = 9; //todo
int analogImdBmsFaultReset = 11;
int analogThermistor1 = 12;
int analogThermistor2 = 13;
int analogThermistor3 = 14;
int analogLVBatt = 15;
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
int i = 0; // for the LV battery read loop
int j = 1; // also for LV battery read loop
int lvbatt[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // for LV battery analog readings
boolean LVBattProblem = false;

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
boolean startupSequencePrinted = false;//So stuff gets printed once per startup stage
int dashSwitch2Val = -1;
int dashButtonVal = -1;
boolean dashButtonPressedMemory = false;//Remembers momentary button press till acted upon
int newLineIndex;

unsigned long test1SecLoop = 0;

void setup() {
    Serial.begin(115200);//Talk back to computer
    Serial1.begin(115200);//ar2
    Serial2.begin(115200);//ar4
    Serial3.begin(115200);//ar5
    inputCmdStream1.reserve(100);
    inputCmdStream2.reserve(100);
    inputCmdStream3.reserve(100);
    //Initialize output pins
    pinMode(digitalRelay1, OUTPUT);
    digitalWrite(digitalRelay1, HIGH);
    pinMode(digitalRelay2, OUTPUT);
    digitalWrite(digitalRelay2, HIGH);
    pinMode(digitalRelay3, OUTPUT);
    digitalWrite(digitalRelay3, HIGH);
    pinMode(digitalRelay4, OUTPUT);
    digitalWrite(digitalRelay4, HIGH);
    pinMode(digitalRelay5, OUTPUT);
    digitalWrite(digitalRelay5, HIGH);
    pinMode(digitalRelay6, OUTPUT);
    digitalWrite(digitalRelay6, HIGH);
    pinMode(digitalRelay7, OUTPUT);
    digitalWrite(digitalRelay7, HIGH);
    pinMode(digitalRelay8, OUTPUT);
    digitalWrite(digitalRelay8, HIGH);
    pinMode(digitalImd, OUTPUT);//todo output or input?
    pinMode(digitalTransistor4, OUTPUT);
    pinMode(digitalBrake, OUTPUT);
    pinMode(digitalTransistor2, OUTPUT);
    pinMode(digitalPumpFan, OUTPUT);

    digitalWrite(digitalPumpFan, HIGH);//Turn on cooling fan

    //Wait 1 second for communication before throwing error
    timeoutRx1 = 5000;
    timeoutRx2 = 5000;
    timeoutRx3 = 5000;
    runLoop = 0;
    Serial1.println();
}

void loop() {

    /*************************************
    Commands that run whether or not car is ready to drive
    *************************************/
    if (stringComplete1) {//Received command from AR2
        newLineIndex = inputCmdStream1.indexOf('\n');
        if (newLineIndex > -1) {//Newline is found
            inputCmd1 = inputCmdStream1.substring(0, newLineIndex);
            inputCmdStream1 = inputCmdStream1.substring(newLineIndex + 1);
        }
        if (inputCmdStream1.indexOf('\n') == -1) {//No more complete commands
            stringComplete1 = false;
        }
        if (inputCmd1 == "ar1:restart") {
            reset(13);
        } else if (inputCmd1.substring(0,10) == "ar1:print:") {
            Serial.print(millis());
            Serial.print(" - ");
            Serial.println(inputCmd1.substring(10));//todo make sure this works
        } else if (inputCmd1.substring(0,17) == "ar1:dashSwitches:") {
            char switchRead = inputCmd1.charAt(17);
            dashSwitch2Val = switchRead - '0';//todo make sure char to int conversion works
            switchRead = inputCmd1.charAt(18);
            dashButtonVal = switchRead - '0';
            if (dashButtonVal = 1) {
                dashButtonPressedMemory = true;
            }
        }
    }

    if (stringComplete2) {//Received command from ar4
        newLineIndex = inputCmdStream2.indexOf('\n');
        if (newLineIndex > -1) {//Newline is found
            inputCmd2 = inputCmdStream2.substring(0, newLineIndex);
            inputCmdStream2 = inputCmdStream2.substring(newLineIndex + 1);
        }
        if (inputCmdStream2.indexOf('\n') == -1) {//No more complete commands
            stringComplete2 = false;
        }
        //Put stuff here
    }
    if (stringComplete3) {//Received command from ar5
        newLineIndex = inputCmdStream3.indexOf('\n');
        if (newLineIndex > -1) {//Newline is found
            inputCmd3 = inputCmdStream3.substring(0, newLineIndex);
            inputCmdStream3 = inputCmdStream3.substring(newLineIndex + 1);
        }
        if (inputCmdStream3.indexOf('\n') == -1) {//No more complete commands
            stringComplete3 = false;
        }
        //Put stuff here
    }
    if (runLoop < millis()) {//Runs 4x per second
        runLoop = millis() + 250;//Push runLoop up 250 ms
        Serial1.print("ar2:throttle:");
        Serial1.println(torqueValAdjusted);

        //todo read motor/controller/water temp - pins A2-A3
        //todo write error code if emergency buttons pressed? - pins A6-A10

        //todo BMS shutdown
        

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
    
    if (test1SecLoop < millis()) {
        test1SecLoop = millis() + 1000;
        
        
        /****************************
        Begin Low Voltage Battery Reading Code
        ***************************/
        j = 0;
        for(i = 0; i < 16; i++) {// turns on the mux pins
          digitalWrite(mux8, LOW);
          digitalWrite(mux4, LOW);
          digitalWrite(mux2, LOW);
          digitalWrite(mux1, LOW);
          if (i == 9 || i == 2 || i == 1 || i == 0){
          }
          else {
            if ((i | 0x08) == 0x08) { // 8s place
              digitalWrite(mux8, HIGH);
            }
            if((i | 0x04) == 0x04) { // 4s place
              digitalWrite(mux4, HIGH);
            }
            if((i | 0x04) == 0x04) { // you get the idea
              digitalWrite(mux2, HIGH);
            }
            if((i | 0x04) == 0x04) {
              digitalWrite(mux1, HIGH);
            }
            delay(1); // let the signal propogate
            lvbatt[11-j] = analogRead(analogLVBatt); // so that results go in low cell to high cell
            if ((lvbatt[j] < 614) && (i == 15 || i == 8)) { // this checks if the lowest cell is too low
              LVBattProblem = true;
            }
              
            
            
            j++;
          }
        }
        for( i = 0; i < 6; i++) {
          lvbatt[i] = lvbatt[i]*(i+1); // so that we get actual voltages
        }
        for( i = 6; i < 12; i++) {
          lvbatt[i] = lvbatt[i]*(i-5); // so that we get actual voltages
        }
        // we don't want any one cell to have less that 3 volts (614 in arduino terms)
        for(i = 1; i < 6; i++) {
          if((lvbatt[i] - lvbatt[i-1]) < 614) { // checks first battery
            LVBattProblem = true;
          }
        }
        for(i = 7; i < 12; i++) {
          if((lvbatt[i] - lvbatt[i-1]) < 614) { // checks second battery
            LVBattProblem = true;
          }
        }
        if(LVBattProblem) {
          Serial.println("LV Battery Fault");
        }
        else {
          Serial.println("LV Battery is fine");
        }
        for(i = 0; i < 12; i++) {
          Serial.print(lvbatt[i]);
          Serial.write(32);
        }
        Serial.println("  ");      
        /*********************************
        End LV Batery Reading Code
        **********************************/
    }

    /*************************************
    End always-run commands
    *************************************/


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
        if (newChar == '\n') {
            stringComplete1 = true;
            timeoutRx1 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            //Serial.println("recvd ar2: "+inputCmd1);
        }
        inputCmdStream1 += newChar;
    }
}

void serialEvent2() {//Receives commands from ar4. todo: might have problems if receives while using inputCmdStream in arduino loop
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {
            stringComplete2 = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            //Serial.println("recvd ar4: "+inputCmd2);
        }
        inputCmdStream2 += newChar;

    }
}

void serialEvent3() {//Receives commands from ar5
    while (Serial3.available()) {
        char newChar = (char)Serial3.read();
        if (newChar == '\n') {
            stringComplete3 = true;
            timeoutRx3 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
            //Serial.println("recvd ar5: "+inputCmd3);
        }
        inputCmdStream3 += newChar;

    }
}

void reset(int errCode) {
    Serial.print(millis());
    Serial.print(" - Shutting down - ");
    Serial.println(errCode);//Now send to computer
    Serial1.println("ar2:restart");
    //todo ar4 & ar5 don't need to reset right?
    ready2Drive = false;
    startupSequence = 0;//Reset the startup sequence
    eepromChecked = false;
    eepromCheckGood = false;
    dashSwitch2Val = -1;//Init values of -1
    dashButtonVal = -1;
}
