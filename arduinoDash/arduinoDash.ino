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
unsigned long errorClearTimestamp1 = 0;
unsigned long errorClearTimestamp2 = 0;
unsigned long errorClearTimestamp3 = 0;
unsigned long resetTimestamp = 0;
unsigned long timeSinceReset = 0;


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
    Serial2.println();
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
            writeError1("Ready to drive");
            errorClearTimestamp1 = millis() + 5000;
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
        } else if (inputCmd2.substring(0,12) == "ar2:startup:") {
            String stage = inputCmd2.substring(12);
            errorClearTimestamp1 = 0;
            if (stage == "0") {
                writeError1("Init switch to neutral");
            } else if (stage == "1") {
                writeError1("Press init button");
            } else if (stage == "2") {
                writeError1("Relays 1,3 closed");
            } else if (stage == "3") {
                writeError1("Relays 2, precharge closed");
            } else if (stage == "5") {
                writeError1("Relay 4 closed, precharge open");
            } else if (stage == "6") {
                writeError1("Init switch to neutral");
            } else if (stage == "7") {
                writeError1("Press init button to start");
            } else if (stage == "8") {
                writeError1("Ready to drive sound active");
            }
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
            initScreen();
            cycleDisplay = 1;
        }

        if (errorClearTimestamp1 != 0 && errorClearTimestamp1 < millis()) {
            writeError1("");
        }
        if (errorClearTimestamp2 != 0 && errorClearTimestamp2 < millis()) {
            writeError2("");
        }
        if (errorClearTimestamp3 != 0 && errorClearTimestamp3 < millis()) {
            writeError3("");
        }

        if (resetTimestamp != 0) {
            timeSinceReset = millis() - resetTimestamp;
            timeSinceReset /= 1000;
            String error3 = "Time since reset: " + String(timeSinceReset);
            writeError3(error3);
        }

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
        writeError2("Serial conn. lost");
        errorClearTimestamp2 = millis() + 10000;//Keep error on screen 10 seconds
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
    tft.fillScreen(0xD5A8);
    tft.setCursor(10, 10);
    tft.setTextColor(0x0000); // black text on old gold screen
    tft.setTextSize(6);
    tft.print("Torque:");
    tft.setCursor(10, 100);
    tft.print("RPM:");
    tft.setTextSize(1);
    tft.setCursor(10, 300);
    tft.print("HyTech Racing 2015. You can't get much more ramblin' than this");
    tft.setTextColor(0x0000);
    tft.setTextSize(6);
}

void reset() {
    ready2Drive = false;
    resetLoop = true;
    errorClearTimestamp1 = millis() + 5000;
    resetTimestamp = millis();
    writeError1("Vehicle reset");
    //todo reset stuff like error LEDs (also make sure ar1 sends errors after this loop runs)
    //todo this part might need to happen on a soft restart below
    digitalWrite(ledPinImdFault, LOW);
    digitalWrite(ledPinBmsFault, LOW);
    digitalWrite(ledPinSwitch1, LOW);
    digitalWrite(ledPinSwitch2, LOW);
    digitalWrite(ledPinSwitch4, LOW);
    digitalWrite(ledPinSwitch5, LOW);
    dashSwitch2Val = -1;
    dashButtonVal = -1;
}

void writeError1(String error) {//Display an error on screen
    if (error.length() > 14) {
        tft.setTextSize(4);
    } else {
        tft.setTextSize(5);
    }
    tft.fillRect(0,150,480,50,0xD5A8);//Fill from 150 to 200
    tft.setCursor(10,155);
    tft.setTextColor(0xF800);
    tft.print(error);
    tft.setTextSize(6);
    tft.setTextColor(0x0000);
}

void writeError2(String error) {
    if (error.length() > 14) {
        tft.setTextSize(4);
    } else {
        tft.setTextSize(5);
    }
    tft.fillRect(0,200,480,50,0xD5A8);//Fill from 200 to 250
    tft.setCursor(10,210);
    tft.setTextColor(0xF800);
    tft.print(error);
    tft.setTextSize(6);
    tft.setTextColor(0x0000);
}

void writeError3(String error) {
    tft.fillRect(0,250,480,50,0xD5A8);//Fill from 250 to 300
    tft.setCursor(10,260);
    tft.setTextSize(3);
    tft.setTextColor(0xF800);
    tft.print(error);
    tft.setTextSize(6);
    tft.setTextColor(0x0000);
}

void checkSwitches() {
    if (dashSwitch2Val != 0 && digitalRead(analogSwitch2a) == HIGH && digitalRead(analogSwitch2b == LOW)) {
        dashSwitch2Val = 0;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch2Val != 1 && digitalRead(analogSwitch2a) == LOW && digitalRead(analogSwitch2b) == LOW) {
        dashSwitch2Val = 1;
        sendSwitchValsThisLoop = true;
    } else if (dashSwitch2Val != 2 && digitalRead(analogSwitch2a) == LOW && digitalRead(analogSwitch2b) == HIGH) {
        dashSwitch2Val = 2;
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
