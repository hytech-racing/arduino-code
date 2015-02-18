/*
Arduino 2
Type: Mega
Use: Read and process pedal values, show values on LCD
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
#include <EEPROM.h>;

int ledPinWaterPumpAlert = 11;
int ledPinImdFault = 12;
int ledPinAmsBmsFault = 13;
int analogCycleDisplay = 8;

//Extended pot is low resistance
//todo set these pins to which pins are being used
//Pot1 and Pot2 used for acceleration
int pot1 = 0;
int pot2 = 1;
int pot3 = 2;//Pot3 used for brake

//todo change these values to actual values once installed
/*
It seems better to find the Arduino mapped values instead of converting into mV
every time the loop runs
Extended = more resistance, Pedal compresses pot when pressed, therefore higher voltage reading corresponds to higher torque
*/
//Pot1 and Pot2 used for acceleration
int pot1High = 0;//Pedal pressed
int pot1Low = 0;//Pedal resting
int pot2High = 0;
int pot2Low = 0;
int pot3High = 0;//Pot3 used for brake
int pot3Low = 0;
int pot3Hard = 0;//Value when hard braking

String accValPrefix = "ar1:acc:";

/*************************************
END CONFIGURATION
*************************************/

String inputCmd = "";
boolean stringComplete = false;
unsigned long timeoutRx;//Stores millisecond value to shut off if communication is lost
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

float bmsPowerValue = 0;//Receives to use with brake plausibility check

char floatBuffer[32];//used to convert string to float
int pot1ValAdjusted = 0;
int pot2ValAdjusted = 0;
int pot3ValAdjusted = 0;
int potAccVal = 0;
int potAccAdjDiff = 0;//Holds the difference between two accelerator readings
int pot1Range = pot1High - pot1Low;//Ranges that each pot will move (used for percentage calcs)
int pot2Range = pot2High - pot2Low;
int pot3Range = pot3High - pot3Low;

float torqueVal = 0;//0-1024 mapped value for torque

boolean regenActive = false;

boolean brakePlausActive = false;//Set to true if brakes actuated && torque encoder > 25%
boolean ready2Drive = false;//Set to false on startup and soft restart
boolean runStartupErrCheck = true;//Runs the error check once at startup/restart
boolean printErrorActive;//Used to stop printing error after a set amount of time
unsigned long printErrLoop;//Used for running print error loop till millis() surpasses this value
int eepromErrCode;

void setup() {
    Serial.begin(115200);//Talk back to computer
    Serial1.begin(115200);//Talk to ar1
    inputCmd.reserve(50);
    pinMode(ledPinWaterPumpAlert, OUTPUT);
    pinMode(ledPinImdFault, OUTPUT);
    pinMode(ledPinAmsBmsFault, OUTPUT);
    //Wait 1 second for communication before throwing error
    timeoutRx = 1000;
    runLoop = 0;
}

void loop() {
    if (!ready2Drive && runStartupErrCheck) {
        eepromErrCode = EEPROM.read(1);
        if (eepromErrCode < 255) {
            //Something is written in EEPROM
            printErrorActive = true;
            printErrLoop = millis() + 5000;//This will make it send error to computer for about 5 seconds
        }
        EEPROM.write(1,255);
        runStartupErrCheck = false;
    }
    if (stringComplete) {//Recieved something on Serial1
        if (inputCmd.substring(0,13) == "ar2:bmsPower:") {
            inputCmd.substring(13,inputCmd.length()-1).toCharArray(floatBuffer,sizeof(floatBuffer));
            bmsPowerValue = atof(floatBuffer);
        }else if (inputCmd == "ar2:restart") {
            //Restarting vehicle
            ready2Drive = false;
        }else if (inputCmd == "ar2:ready2Drive") {
            ready2Drive = true;
        }else if (inputCmd == "ar2:waterPumpLed:1") {
            digitalWrite(ledPinWaterPumpAlert, HIGH);
        }else if (inputCmd == "ar2:waterPumpLed:0") {
            digitalWrite(ledPinWaterPumpAlert, LOW);
        }else if (inputCmd == "ar2:imdFaultLed:1") {
            digitalWrite(ledPinImdFault, HIGH);
        }else if (inputCmd == "ar2:imdFaultLed:0") {
            digitalWrite(ledPinImdFault, LOW);
        }else if (inputCmd == "ar2:amsBmsFaultLed:1") {
            digitalWrite(ledPinAmsBmsFault, HIGH);
        }else if (inputCmd == "ar2:amsBmsFaultLed:0") {
            digitalWrite(ledPinAmsBmsFault, LOW);
        }
        inputCmd = "";
        stringComplete = false;
    }
    if (runLoop < millis()) {//Runs whether or not car is ready to drive
        if (!ready2Drive) {
            runLoop = millis() + 100;//Push runLoop up 100 ms ONLY if car is not ready to drive (otherwise next top-level if loop will increment runLoop)
        }
        if (printErrorActive) {//Print shutdown error to computer serial
            if (printErrLoop > millis()) {
                Serial.println("Shutoff error code: " + eepromErrCode);
            }else {
              printErrorActive = false;//Error finished printing
            }
        }
        if (analogRead(analogCycleDisplay) > 1000) {//If cycle display button is pressed
            //todo Output various stuff to lcd
        }
        pot1ValAdjusted = (1000/pot1Range)*(analogRead(pot1)-pot1Low);//Make adjusted mapped values (now 0-1000)
        pot2ValAdjusted = (1000/pot2Range)*(analogRead(pot2)-pot2Low);
        pot3ValAdjusted = (1000/pot3Range)*(analogRead(pot3)-pot3Low);

        if(pot3ValAdjusted < pot3High) { //Brake light
            Serial1.println("ar1:brake:1");
        }else {
            Serial1.println("ar1:brake:0");
        }
    }
    if (runLoop < millis() && ready2Drive){//Runs only if car is ready to drive
        runLoop = millis() + 100;//Push runLoop up 100 ms

        potAccAdjDiff = abs(pot2ValAdjusted-pot1ValAdjusted);//Get difference between torque sensors
        if(potAccAdjDiff > 100){//Acceleration error check (Die if 10%+ difference between readings)
            sendHardShutdown(2);//2 means acceleration implausibility
        }else {
            torqueVal = (int)((pot1ValAdjusted + pot2ValAdjusted)/2);
            if(pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
                brakePlausActive = true;
            }else {
                if(brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                    brakePlausActive = false;
                }
                if(!brakePlausActive) {//If brake plausibility is not active
                    //todo send torqueVal to motor controller
                }else {//If brake plausibility is active
                    //todo send 0 to motor controller
                }
            }
        }
        if(bmsPowerValue >= 5){//If brake is pressed hard and BMS says 5kW of power (BMS arduino needs to send that value to this arduino)
            sendHardShutdown(3);//3 means too much power
            //todo might not be receiving value in kW units
        }
        if(false && regenActive == false){//Regeneration //todo what activates this?
            regenActive = true;
            Serial1.println("ar1:regen:1");
            //todo send to motor controller, not rear arduino
        }else if(regenActive==true){
            regenActive = false;
            Serial1.println("ar1:regen:0");
            //todo send to motor controller, not rear arduino
        }

    }
    if (timeoutRx < millis()) {//If 1 second has passed since receiving a complete command send shutdown command
        Serial.println(millis());
        Serial.println("ar2 lost connection to ar1");
        sendHardShutdown(10);//10 means lost communication
        //todo this might make no sense because it is sending a shutdown signal over serial when communication is messed up
    }
}

void Serial1Event() {
    while (Serial1.available()) {
        char newChar = (char)Serial1.read();
        if (newChar == '\n') {//NOTE: inputCmd does NOT include \n
            stringComplete = true;
            timeoutRx = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }else {
            inputCmd += newChar;
        }
    }
}

void sendHardShutdown(int errCode) {
    /*
    Resetable error codes:
    10. Lost communication
    11. Acceleration implausibility
    12. Too much power (>=5kW)
    Only for our purposes, these shutoffs ARE driver resetable
    */
    String shutdownError = "ar1:restart";
    EEPROM.write(1,errCode);//Write resetable codes to addr 1
    Serial1.println(shutdownError);//Send to ar1 first
    Serial.println(shutdownError + ":" + errCode);//Now send to computer
    ready2Drive = false;
    runStartupErrCheck = true;
}
