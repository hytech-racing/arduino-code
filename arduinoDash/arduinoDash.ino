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
int oldCycleDisplay = 0; // so we know if there is a change in cycle display
/**************************** end screen config */

int ledPinWaterPumpAlert = 11;
int ledPinImdFault = 12;
int ledPinAmsBmsFault = 13;
int ledPinStartup1 = 14;
int ledPinStartup2 = 15;
int ledPinStartup3 = 16;
int analogCycleDisplay = 8;
int analogSwitch1a = 22;
int analogSwitch1b = 24;
int analogSwitch2 = 26;
int analogSwitch3a = 28;
int analogSwitch3b = 30;
int analogSwitch4a = 32;
int analogSwitch4b = 34;

/*************************************
END CONFIGURATION
*************************************/

String inputCmdStream1 = "";
boolean stringComplete1 = false;
String inputCmd1;
unsigned long timeoutRx1;//Stores millisecond value to shut off if communication is lost
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

int cycleDisplay = 1;//Stores which view to send to LCD

int torqueVal = 0;
int rpmVal = 0; // not sure if we will read in the values for RPM or not

boolean ready2Drive = false;//Set to false on startup and soft restart
int dashSwitch1Val = -1;
int dashSwitch2Val = -1;
int dashSwitch3Val = -1;
int dashSwitch4Val = -1;
boolean sendSwitchValsThisLoop = false;

void setup() {
    Serial1.begin(115200);//Talk to ar1
    inputCmdStream1.reserve(50);
    pinMode(ledPinWaterPumpAlert, OUTPUT);
    pinMode(ledPinImdFault, OUTPUT);
    pinMode(ledPinAmsBmsFault, OUTPUT);
    pinMode(ledPinStartup1, OUTPUT);
    pinMode(ledPinStartup2, OUTPUT);
    pinMode(ledPinStartup3, OUTPUT);
    //Wait 1 second for communication before throwing error
    timeoutRx1 = 1000;
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
        if (newLineIndex > -1) {//Newline is found
            inputCmd1 = inputCmdStream1.substring(0, newLineIndex - 1);
            inputCmdStream1 = inputCmdStream1.substring(newLineIndex + 2);
        }
        if (inputCmdStream1.indexOf('\n') == -1) {//No more complete commands
            stringComplete1 = false;
        }
        if (inputCmd1.substring(0,11) == "ar2:restart") {
            //Restarting vehicle
            reset();
        } else if (inputCmd1 == "ar2:ready2Drive") {
            ready2Drive = true;
        } else if (inputCmd1 == "ar2:hi") {
            Serial1.print("ar1:hi");
        } else if (inputCmd1 == "ar2:waterPumpLed:1") {
            digitalWrite(ledPinWaterPumpAlert, HIGH);
        } else if (inputCmd1 == "ar2:waterPumpLed:0") {
            digitalWrite(ledPinWaterPumpAlert, LOW);
        } else if (inputCmd1 == "ar2:imdFaultLed:1") {
            digitalWrite(ledPinImdFault, HIGH);
        } else if (inputCmd1 == "ar2:imdFaultLed:0") {
            digitalWrite(ledPinImdFault, LOW);
        } else if (inputCmd1 == "ar2:amsBmsFaultLed:1") {
            digitalWrite(ledPinAmsBmsFault, HIGH);
        } else if (inputCmd1 == "ar2:amsBmsFaultLed:0") {
            digitalWrite(ledPinAmsBmsFault, LOW);
        } else if (inputCmd1 == "ar2:startupLed1:1") {
            digitalWrite(ledPinStartup1, HIGH);
        } else if (inputCmd1 == "ar2:startupLed1:0") {
            digitalWrite(ledPinStartup1, LOW);
        } else if (inputCmd1 == "ar2:startupLed2:1") {
            digitalWrite(ledPinStartup2, HIGH);
        } else if (inputCmd1 == "ar2:startupLed2:0") {
            digitalWrite(ledPinStartup2, LOW);
        } else if (inputCmd1 == "ar2:startupLed3:1") {
            digitalWrite(ledPinStartup3, HIGH);
        } else if (inputCmd1 == "ar2:startupLed3:0") {
            digitalWrite(ledPinStartup3, LOW);
        } else if (inputCmd1.substring(0,13) == "ar2:throttle:") {
            torqueVal = inputCmd1.substring(13).toInt();
        } else if (inputCmd1.substring(0,10) == "ar2:temp1:") {
            //todo put temperatures on screen
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
            tft.print(torqueVal);
            tft.setCursor(298, 160);
            tft.print(rpmVal); // if we ever get an rpm value

        } else if (cycleDisplay == 2) {
            //need to decide on an alternate display
            tft.setCursor(298, 10);
            tft.print(torqueVal);
            tft.setCursor(298, 160);
            tft.print(rpmVal); // if we ever get an rpm value
        }
    }
}

void serialTimeout() {
    if (timeoutRx1 < millis()) {//If 1 second has passed since receiving a complete command from main Arduino reset ***SOMETHING HAS GONE WRONG***
        Serial.print("ar1:print:");
        Serial.println(millis());
        Serial1.println("ar1:print:ar2 lost connection to ar1");
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
        } else {
            inputCmdStream1 += newChar;
        }
    }
}

void initScreen() {
    tft.setRotation(3);
    tft.fillScreen(0xFFFF);
    tft.setCursor(10, 10);
    tft.setTextColor(0x0000, 0xD5A8); // black text on old gold screen
    tft.setTextSize(6);
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
    digitalWrite(ledPinStartup1, LOW);
    digitalWrite(ledPinStartup2, LOW);
    digitalWrite(ledPinStartup3, LOW);
}
