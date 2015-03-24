/*
Arduino 2
Type: Mega
Use: Show values on LCD
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
************************ screen config
#include <Adafruit_GFX.h>
#include <Adafruit_HX8357.h>
#include <SPI.h>
#include <stdint.h>
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
int oldCtcleDisplay = 0; // so we know if there is a change in cycle display
**************************** end screen config

int ledPinWaterPumpAlert = 11;
int ledPinImdFault = 12;
int ledPinAmsBmsFault = 13;
int analogCycleDisplay = 8;

/*************************************
END CONFIGURATION
*************************************/

String inputCmd1 = "";
String inputCmd2 = "";
boolean stringComplete1 = false;
boolean stringComplete2 = false;
unsigned long timeoutRx1;//Stores millisecond value to shut off if communication is lost
unsigned long timeoutRx2;
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

int cycleDisplay = 1;//Stores which view to send to LCD

float torqueVal;//0-1000 mapped value for torque
float torqueValAdjusted;//Adjusted exponentially

int rpmVal = 0; // not sure if we will read in the values for RPM or not

boolean regenActive = false;

boolean brakePlausActive = false;//Set to true if brakes actuated && torque encoder > 25%
boolean ready2Drive = false;//Set to false on startup and soft restart
boolean resetRun = true;//Set to false on soft restart (turns true after this arduino resets stuff in its scope)

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
    initScreen() // function defined below
}

void loop() {

    if (!ready2Drive && !resetRun) {
        //todo reset stuff like error LEDs (also make sure ar1 sends errors after this loop runs)
        //todo this part might need to happen on a soft restart below
        digitalWrite(ledPinWaterPumpAlert, LOW);
        digitalWrite(ledPinImdFault, LOW);
        digitalWrite(ledPinAmsBmsFault, LOW);
        resetRun = true;
    }

    if (stringComplete1) {//Recieved something from ar1
        if (inputCmd1.substring(0,11) == "ar2:restart") {
            //Restarting vehicle
            ready2Drive = false;
            //todo show error on screen
        }else if (inputCmd1.subtring(0,4) == "ar3:") {
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
        inputCmd1 = "";
        stringComplete1 = false;
    }

    if (stringComplete2) {//Received something from ar3
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
        inputCmd2 = "";
        stringComplete2 = false;
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

    }
}

void serialTimeout() {
    if (timeoutRx1 < millis() || timeoutRx2 < millis()) {//If 1 second has passed since receiving a complete command from both Arduinos reset ***SOMETHING HAS GONE WRONG***
        Serial.print("ar1:print:")
        Serial.println(millis());
        if(timeoutRx1 < millis()) {
            Serial1.println("ar2 lost connection to ar1");
        }
        if(timeoutRx2 < millis()) {
            Serial1.println("ar2 lost connection to ar3");
        }
        Serial1.println("ar1:restart");
        ready2Drive = false;
        runStartupErrCheck = true;

        tft.setTextColor(0xF800);
        tft.setCursor(298, 160);
        tft.print("Serial connection lost");
    }
}

void SerialEvent1() {
    while (Serial1.available()) {
        char newChar = (char)Serial1.read();
        if (newChar == '\n') {//NOTE: inputCmd1 does NOT include \n
            stringComplete = true;
            timeoutRx1 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }else {
            inputCmd1 += newChar;
        }
    }
}

void SerialEvent2() {
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {//NOTE: inputCmd2 does NOT include \n
            stringComplete = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }else {
            inputCmd2 += newChar;
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
