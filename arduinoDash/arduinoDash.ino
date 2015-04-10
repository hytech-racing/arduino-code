/*
Arduino 2
Type: Mega
Use: Show values on LCD
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
/************************ screen config */
#include <Adafruit_GFX.h>
#include <Adafruit_HX8357.h>
#include <SPI.h>
#include <stdint.h>
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
int oldCtcleDisplay = 0; // so we know if there is a change in cycle display
/**************************** end screen config */

int ledPinWaterPumpAlert = 11;
int ledPinImdFault = 12;
int ledPinAmsBmsFault = 13;
int analogCycleDisplay = 8;
int analogSwitch1a = 22;
int analogSwitch1b = 24;
int analogSwitch2 = 26;
int analogSwitch3a = 28;
int analogSwitch3b = 30;
int analogSwitch4a = 32;
int analogSwitch4b = 34;

int pot1 = 0;//Pot1 and Pot2 used for acceleration
int pot2 = 1;
int pot3 = 2;//Pot3 used for brake
int pot1High = 376;//Pedal pressed
int pot1Low = 147;//Pedal resting//todo right now the low vals are when a little pressure is applied
int pot2High = 462;
int pot2Low = 226;
int pot3High = 468;//Pot3 used for brake
int pot3Low = 336;
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

/*************************************
END CONFIGURATION
*************************************/

String inputCmdStream1 = "";
String inputCmdStream2 = "";
boolean stringComplete1 = false;
boolean stringComplete2 = false;
unsigned long timeoutRx1;//Stores millisecond value to shut off if communication is lost
unsigned long timeoutRx2;
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

int cycleDisplay = 1;//Stores which view to send to LCD

int rpmVal = 0; // not sure if we will read in the values for RPM or not

boolean ready2Drive = false;//Set to false on startup and soft restart
int dashSwitch1Val = -1;
int dashSwitch2Val = -1;
int dashSwitch3Val = -1;
int dashSwitch4Val = -1;
boolean sendSwitchValsThisLoop = false;

void setup() {
    Serial1.begin(115200);//Talk to ar1
    Serial2.begin(115200);//Talk to ar3
    inputCmd1.reserve(50);
    inputCmd2.reserve(50);
    pinMode(ledPinWaterPumpAlert, OUTPUT);
    pinMode(ledPinImdFault, OUTPUT);
    pinMode(ledPinAmsBmsFault, OUTPUT);
    //Wait 1 second for communication before throwing error
    timeoutRx1 = 1000;
    timeoutRx2 = 1000;
    runLoop = 0;
    initScreen(); // function defined below
}

void loop() {
    if (dashSwitch1Val != 0 && analogRead(analogSwitch1a) > 500) {//Change to low (0)
        dashSwitch1Val = 0;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch1Val != 2 && analogRead(analogSwitch1b) > 500) {//Change to high (2)
        dashSwitch1Val = 2;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch1Val != 1 && analogRead(analogSwitch1a) < 500 && analogRead(analogSwitch1b) < 500) {//Change to neutral (1)
        dashSwitch1Val = 1;
        sendSwitchValsThisLoop = true;
    }
    if (dashSwitch2Val != 1 && analogRead(analogSwitch2) > 500) {//Change to low (0)
        dashSwitch2Val = 1;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch2Val != 0 && analogRead(analogSwitch2) < 500) {//Change to high (1)
        dashSwitch2Val = 0;
        sendSwitchValsThisLoop = true;
    }
    if (dashSwitch3Val != 0 && analogRead(analogSwitch3a) > 500) {//Change to low (0)
        dashSwitch3Val = 0;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch3Val != 2 && analogRead(analogSwitch3b) > 500) {//Change to high (2)
        dashSwitch3Val = 2;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch3Val != 1 && analogRead(analogSwitch3a) < 500 && analogRead(analogSwitch3b) < 500) {//Change to neutral (1)
        dashSwitch3Val = 1;
        sendSwitchValsThisLoop = true;
    }
    if (dashSwitch4Val != 0 && analogRead(analogSwitch4a) > 500) {//Change to low (0)
        dashSwitch4Val = 0;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch4Val != 2 && analogRead(analogSwitch4b) > 500) {//Change to high (2)
        dashSwitch4Val = 2;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch4Val != 1 && analogRead(analogSwitch4a) < 500 && analogRead(analogSwitch4b) < 500) {//Change to neutral (1)
        dashSwitch4Val = 1;
        sendSwitchValsThisLoop = true;
    }
    if (sendSwitchValsThisLoop) {
        Serial1.print("ar1:dashSwitches:");
        Serial1.print(dashSwitch1Val);
        Serial1.print(dashSwitch2Val);
        Serial1.print(dashSwitch3Val);
        Serial1.println(dashSwitch4Val);
        sendSwitchValsThisLoop = false;
    }

    if (stringComplete1) {//Recieved something from ar1
        int newLineIndex = inputCmdStream1.indexOf('/n');
        if (newLineIndex > -1) {
            String inputCmd1 = inputCmdStream1.substring(0,newLineIndex);
            inputCmdStream1 = inputCmdStream1.substring(newLineIndex + 1);
        } else {
            String inputCmd1 = inputCmdStream1;
            stringComplete1 = false;
        }
        if (inputCmd1.substring(0,11) == "ar2:restart") {
            //Restarting vehicle
            reset();
        }else if (inputCmd1.substring(0,4) == "ar3:") {
            Serial2.println(inputCmd1);
        }else if (inputCmd1 == "ar2:ready2Drive") {
            ready2Drive = true;
        }else if (inputCmd1 == "ar2:waterPumpLed:1") {
            digitalWrite(ledPinWaterPumpAlert, HIGH);
        }else if (inputCmd1 == "ar2:waterPumpLed:0") {
            digitalWrite(ledPinWaterPumpAlert, LOW);
        }else if (inputCmd1 == "ar2:imdFaultLed:1") {
            digitalWrite(ledPinImdFault, HIGH);
        }else if (inputCmd1 == "ar2:imdFaultLed:0") {
            digitalWrite(ledPinImdFault, LOW);
        }else if (inputCmd1 == "ar2:amsBmsFaultLed:1") {
            digitalWrite(ledPinAmsBmsFault, HIGH);
        }else if (inputCmd1 == "ar2:amsBmsFaultLed:0") {
            digitalWrite(ledPinAmsBmsFault, LOW);
        }
    }

    if (stringComplete2) {//Received something from ar3
        int newLineIndex = inputCmdStream2.indexOf('/n');
        if (newLineIndex > -1) {
            String inputCmd2 = inputCmdStream2.substring(0,newLineIndex);
            inputCmdStream2 = inputCmdStream2.substring(newLineIndex + 1);
        } else {
            String inputCmd2 = inputCmdStream2;
            stringComplete2 = false;
        }
        if (inputCmd2.substring(0,4) == "ar1:") {
            Serial1.println(inputCmd2);
        }else if (inputCmd2 == "ar2:accelImplaus:1") {
            //todo put error on screen?
        }else if (inputCmd2 == "ar2:accelImplaus:0") {
            //todo remove error from screen?
        }else if (inputCmd2 == "ar2:brakePlaus:1") {
            //todo put error on screen?
        }else if (inputCmd2 == "ar2:brakePlaus:0") {
            //todo remove error from screen?
        }
    }

    if (runLoop < millis()) {//Runs whether or not car is ready to drive
        runLoop = millis() + 100;//Push runLoop up 100 ms

        if (analogRead(analogCycleDisplay) > 1000) {//If cycle display button is pressed
            if (cycleDisplay == 1) {
                cycleDisplay = 2;
            } else if (cycleDisplay == 2) {
                cycleDisplay = 1;
            }
        }

        //Update LCD todo this needs to be fleshed out
        if (cycleDisplay == 1) {//Default view
            tft.setCursor(298, 10);
            tft.print(torqueValAdjusted);
            tft.setCursor(298, 160);
            tft.print(rpmVal); // if we ever get an rpm value

        } else if (cycleDisplay == 2) {
            //need to decide on an alternate display
            tft.setCursor(298, 10);
            tft.print(torqueValAdjusted);
            tft.setCursor(298, 160);
            tft.print(rpmVal); // if we ever get an rpm value
        }

        /******************************
        Throttle reading code
        ******************************/
        //Read analog values all at once
        pot1ValAdjusted = analogRead(pot1);
        pot2ValAdjusted = analogRead(pot2);
        pot3ValAdjusted = analogRead(pot3);

        if (pot1ValAdjusted > 1000 || pot1ValAdjusted < 10 || pot2ValAdjusted > 1000 || pot2ValAdjusted < 10) {
            Serial1.println("ar1:restart");
            Serial1.println("ar1:print:Throttle encoder short detected");
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
        Serial.print("ar1:print:Throttle value ");
        Serial.println(torqueValAdjusted);//Prints torque value to computer

        if (pot3ValAdjusted > 0) { //Brake light
            Serial.println("ar1:brake:1");
            Serial.println("ar1:print:Brake lights on");
        } else {
            Serial.println("ar1:brake:0");
            Serial.println("ar1:print:Brake lights off");
        }

        if (potAccAdjDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
            //todo error checking which can detect open circuit, short to ground and short to sensor power
            //todo does this need to shut down car or just send 0 torque val?
            //todo put error on screen
            Serial.println("ar1:print:Acceleration Implausibility on");
        } else {
            //todo remove error from screen
            Serial.println("ar1:print:Acceleration Implausibility off");
            if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
                brakePlausActive = true;
                //todo put error on screen
                //todo put 0 throttle on screen
                Serial.println("ar1:print:Brake plausibility on");
            } else {
                if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                    brakePlausActive = false;
                    //todo remove error from screen
                    Serial.println("ar1:print:Brake plausibility off");
                }
                if (!brakePlausActive) {//If brake plausibility is not active
                    //todo put throttle on screen
                } else {//If brake plausibility is active
                    //todo put 0 throttle on screen
                }
            }
        }
    }
}

void serialTimeout() {
    if (timeoutRx1 < millis() || timeoutRx2 < millis()) {//If 1 second has passed since receiving a complete command from both Arduinos reset ***SOMETHING HAS GONE WRONG***
        Serial.print("ar1:print:");
        Serial.println(millis());
        if(timeoutRx1 < millis()) {
            Serial1.println("ar2 lost connection to ar1");
        }
        if(timeoutRx2 < millis()) {
            Serial1.println("ar2 lost connection to ar3");
        }
        Serial1.println("ar1:restart");
        reset();

        tft.setTextColor(0xF800);
        tft.setCursor(298, 160);
        tft.print("Serial connection lost");
    }
}

void SerialEvent1() {
    while (Serial1.available()) {
        char newChar = (char)Serial1.read();
        if (newChar == '\n') {//NOTE: inputCmd1 does NOT include \n
            stringComplete1 = true;
            timeoutRx1 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }else {
            inputCmdStream1 += newChar;
        }
    }
}

void SerialEvent2() {
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {//NOTE: inputCmd2 does NOT include \n
            stringComplete2 = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }else {
            inputCmdStream2 += newChar;
        }
    }
}

void initScreen() {
    tft.setRotation(3);
    tft.fillScreen(0xFFFF);
    tft.setCursor(uint16_t 10, uint16_t 10);
    tft.setTextColor(uint16_t 0x0000, uint16_t 0xD5A8); // black text on old gold screen
    tft.setTextSize(uint8_t 6);
    tft.print("Torque: ");
    tft.setCursor(80, 118);
    tft.print("RPM: ");
    tft.setCursor(118, 160);
    tft.print("ERR ");
    tft.setTextSize(1);
    tft.setCursor(10, 300);
    tft.setTextColor(0xFFFF, 0xD5A8);
    tft.print("HyTech Racing 2015. You can't get much more ramblin' than this");
    tft.setTextColor(0x0000, 0xD5A8);
    tft.setTextSize(0x06);
}

void reset() {
    ready2Drive = false;
    //todo show error on screen
    //todo reset stuff like error LEDs (also make sure ar1 sends errors after this loop runs)
    //todo this part might need to happen on a soft restart below
    digitalWrite(ledPinWaterPumpAlert, LOW);
    digitalWrite(ledPinImdFault, LOW);
    digitalWrite(ledPinAmsBmsFault, LOW);
}
