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
/**************************** end screen config */

int ledPinImdFault = 12;
int ledPinBmsFault = 11;
int ledPinSwitch1 = 14;
int ledPinSwitch2 = 15;
//int ledPinButton; (Not connected)
int ledPinSwitch4 = 14;
int ledPinSwitch5 = 15;
//int analogSwitch1a; (Not connected)
//int analogSwitch1b; (Not connected)
int analogSwitch2a = 2;
int analogSwitch2b = 5;
int analogButton = 6;
//int analogSwitch3a; (Not connected)
//int analogSwitch3b; (Not connected)
//int analogSwitch4a; (Not connected)
//int analogSwitch4b; (Not connected)
int analogButtonCycleDisplay = 7;


/*************************************
END CONFIGURATION
*************************************/

String inputCmdStream2 = "";
boolean stringComplete2 = false;
String inputCmd2;
unsigned long timeoutRx2;//Stores millisecond value to shut off if communication is lost
unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

int cycleDisplay = 1;//Stores which view to send to LCD

int temp1Val = -1;
int torqueVal = -1;
int rpmVal = -1; // not sure if we will read in the values for RPM or not

boolean ready2Drive = false;//Set to false on startup and soft restart
boolean resetLoop = false;//Run stuff after restart
unsigned long resetTimestamp = 0;

int dashSwitch2Val = -1;
int dashButtonVal = -1;
boolean sendSwitchValsThisLoop = false;

void setup() {
    Serial2.begin(115200);//Talk to ar1
    inputCmdStream2.reserve(50);
    pinMode(ledPinImdFault, OUTPUT);
    pinMode(ledPinBmsFault, OUTPUT);
    pinMode(ledPinSwitch1, OUTPUT);
    pinMode(ledPinSwitch2, OUTPUT);
    pinMode(ledPinSwitch4, OUTPUT);
    pinMode(ledPinSwitch5, OUTPUT);
    pinMode(analogSwitch2a, INPUT);
    pinMode(analogSwitch2b, INPUT);
    pinMode(analogButton, INPUT);
    pinMode(analogButtonCycleDisplay, INPUT);

    //Wait 1 second for communication before throwing error
    timeoutRx2 = 1000;
    runLoop = 0;
    tft.begin(HX8357D);
    initScreen(); // function defined below
}

void loop() {
    checkSwitches();
    SerialEvent2();
    if (stringComplete2) {//Recieved something from ar1
        int newLineIndex = inputCmdStream2.indexOf('\n');
        if (newLineIndex > -1) {//Newline is found
            inputCmd2 = inputCmdStream2.substring(0, newLineIndex - 1);
            inputCmdStream2 = inputCmdStream2.substring(newLineIndex + 1);
        }
        if (inputCmdStream2.indexOf('\n') == -1) {//No more complete commands
            stringComplete2 = false;
        }
        if (inputCmd2.substring(0,11) == "ar2:restart") {
            //Restarting vehicle
            reset();
        } else if (inputCmd2 == "ar2:ready2Drive") {
            ready2Drive = true;
        } else if (inputCmd2 == "ar2:hi") {
            Serial1.print("ar1:hi");
        } else if (inputCmd2 == "ar2:imdFaultLed:1") {
            digitalWrite(ledPinImdFault, HIGH);
        } else if (inputCmd2 == "ar2:imdFaultLed:0") {
            digitalWrite(ledPinImdFault, LOW);
        } else if (inputCmd2 == "ar2:bmsFaultLed:1") {
            digitalWrite(ledPinBmsFault, HIGH);
        } else if (inputCmd2 == "ar2:bmsFaultLed:0") {
            digitalWrite(ledPinBmsFault, LOW);
        } else if (inputCmd2 == "ar2:ledSwitch1:1") {
            digitalWrite(ledPinSwitch1, HIGH);
        } else if (inputCmd2 == "ar2:ledSwitch1:0") {
            digitalWrite(ledPinSwitch1, LOW);
        } else if (inputCmd2 == "ar2:ledSwitch2:1") {
            digitalWrite(ledPinSwitch2, HIGH);
        } else if (inputCmd2 == "ar2:ledSwitch2:0") {
            digitalWrite(ledPinSwitch2, LOW);
        } else if (inputCmd2 == "ar2:ledSwitch4:1") {
            digitalWrite(ledPinSwitch4, HIGH);
        } else if (inputCmd2 == "ar2:ledSwitch4:0") {
            digitalWrite(ledPinSwitch4, LOW);
        } else if (inputCmd2 == "ar2:ledSwitch5:1") {
            digitalWrite(ledPinSwitch5, HIGH);
        } else if (inputCmd2 == "ar2:ledSwitch5:0") {
            digitalWrite(ledPinSwitch5, LOW);
        } else if (inputCmd2.substring(0,13) == "ar2:throttle:") {
            torqueVal = inputCmd2.substring(13).toInt();
        } else if (inputCmd2.substring(0,10) == "ar2:temp1:") {
            //todo put temperatures on screen
            temp1Val = inputCmd2.substring(10).toInt();
        }
    }
    if (runLoop < millis()) {//Runs whether or not car is ready to drive
        runLoop = millis() + 100;//Push runLoop up 100 ms

        if (digitalRead(analogButtonCycleDisplay) == HIGH) {//If cycle display button is pressed
            if (cycleDisplay == 1) {
                cycleDisplay = 2;
            } else if (cycleDisplay == 2) {
                cycleDisplay = 1;
            }
        }

        //Update LCD todo this needs to be fleshed out
        if (cycleDisplay == 1) {//Default view
            tft.setCursor(270, 10);
            tft.print(torqueVal);
            tft.setCursor(270, 100);
            tft.print(rpmVal); // if we ever get an rpm value

        } else if (cycleDisplay == 2) {
            //need to decide on an alternate display
            tft.setCursor(280, 10);
            tft.print(torqueVal);
            tft.setCursor(280, 100);
            tft.print(rpmVal); // if we ever get an rpm value
        }
        
        /*if (resetLoop && resetTimestamp + 5000 < millis() ) {
            initScreen();
            resetLoop = false;
        }*/
        
        serialTimeout();
    }
}

void serialTimeout() {
    if (timeoutRx2 < millis()) {//If 1 second has passed since receiving a complete command from main Arduino reset ***SOMETHING HAS GONE WRONG***
        Serial2.print("ar1:print:");
        Serial2.println(millis());
        Serial2.println("ar1:print:ar2 lost connection to ar1");
        Serial2.println("ar1:restart");
        reset();

        tft.setTextColor(0xF800);
        tft.setCursor(10, 265);
        tft.setTextSize(4);
        tft.print("Serial conn. lost");
        tft.setTextSize(6);//Put size and color back to normal
        tft.setTextColor(0x0000);
    }
}

void SerialEvent2() {
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {
            stringComplete2 = true;
            timeoutRx2 = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
        }
        inputCmdStream2 += newChar;
    }
}

void initScreen() {
    tft.setRotation(1);
    tft.fillScreen(0xFFFF);
    tft.setCursor(10, 10);
    tft.setTextColor(0x0000, 0xD5A8); // black text on old gold screen
    tft.setTextSize(6);
    tft.print("Torque: ");
    tft.setCursor(10, 100);
    tft.print("RPM: ");
    /*tft.setCursor(10, 160);
    tft.print("ERR ");*/
    tft.setTextSize(1);
    tft.setCursor(10, 300);
    tft.setTextColor(0xFFFF, 0xD5A8);
    tft.print("HyTech Racing 2015. You can't get much more ramblin' than this");
    tft.setTextColor(0x0000, 0xD5A8);
    tft.setTextSize(6);
}

void reset() {
    ready2Drive = false;
    resetLoop = true;
    resetTimestamp = millis();
    tft.setCursor(10, 200);
    tft.setTextSize(5);
    tft.print("Vehicle reset");
    tft.setTextSize(6);//Put size and color back to normal
    tft.setTextColor(0x0000);
    //todo reset stuff like error LEDs (also make sure ar1 sends errors after this loop runs)
    //todo this part might need to happen on a soft restart below
    digitalWrite(ledPinImdFault, LOW);
    digitalWrite(ledPinBmsFault, LOW);
    digitalWrite(ledPinSwitch1, LOW);
    digitalWrite(ledPinSwitch2, LOW);
    digitalWrite(ledPinSwitch4, LOW);
    digitalWrite(ledPinSwitch5, LOW);
}

void checkSwitches() {
    if (dashSwitch2Val != 1 && digitalRead(analogSwitch2a) == HIGH) {
        dashSwitch2Val = 1;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch2Val != 0 && digitalRead(analogSwitch2a) == LOW) {
        dashSwitch2Val = 0;
        sendSwitchValsThisLoop = true;
    }
    if (dashButtonVal != 1 && digitalRead(analogButton) == HIGH) {
        dashButtonVal = 1;
        sendSwitchValsThisLoop = true;
    } else if (dashButtonVal != 0 && digitalRead(analogButton) == LOW) {
        dashButtonVal = 0;
        sendSwitchValsThisLoop = true;
    }

    if (sendSwitchValsThisLoop) {
        Serial2.print("ar1:dashSwitches:");
        Serial2.print(dashSwitch2Val);
        Serial2.println(dashButtonVal);
        sendSwitchValsThisLoop = false;

        Serial.print("ar1:dashSwitches:");
        Serial.print(dashSwitch2Val);
        Serial.println(dashButtonVal);
    }
}
