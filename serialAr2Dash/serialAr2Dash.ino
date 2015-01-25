/*
Arduino 2
Type: Mega
Use: Read and process pedal values, show values on LCD
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
#include <EEPROM.h>;

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
Extended = more resistance, Pedal compresses pot when pressed, therefore higher voltage = higher torque
*/
//Pot1 and Pot2 used for acceleration
int pot1High = 0;//Pedal pressed
int pot1Low = 0;//Pedal resting
int pot2High = 0;
int pot2Low = 0;
int pot3High = 0;//Pot3 used for brake
int pot3Low = 0;
int pot3Hard = 0;//Value when hard braking

int ledPinErr = 13;

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

void setup() {
    Serial.begin(115200);//Talk back to computer
    Serial1.begin(115200);//Talk to ar1
    inputCmd.reserve(50);
    pinMode(ledPinErr,OUTPUT);
    //Wait 1 second for communication before throwing error
    timeoutRx = 1000;
    runLoop = 0;
}

void loop() {
    if (stringComplete) {//Recieved something on Serial1
        if(inputCmd.substring(0,13) == "ar2:bmsPower:"){
            inputCmd.substring(13,inputCmd.length()-1).toCharArray(floatBuffer,sizeof(floatBuffer));
            bmsPowerValue = atof(floatBuffer);
        }
        inputCmd = "";
        stringComplete = false;
    }
    if(runLoop < millis()){//Runs up to 10x per second
        runLoop = millis() + 100;//Push runLoop up 100 ms
        pot1ValAdjusted = (1000/pot1Range)*(analogRead(pot1)-pot1Low);//Make adjusted mapped values (now 0-1000)
        pot2ValAdjusted = (1000/pot2Range)*(analogRead(pot2)-pot2Low);
        pot3ValAdjusted = (1000/pot3Range)*(analogRead(pot3)-pot3Low);

        if(pot3ValAdjusted < pot3High) { //Brake light
            Serial1.println("ar1:brake:1");
        }else {
            Serial1.println("ar1:brake:0");
        }

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
        //todo brake over travel?
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
        sendHardShutdown(1);//1 means lost communication
        //todo this might make no sense because it is sending a shutdown signal when communication is messed up
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
    Error codes:
    1. Lost communication
    2. Acceleration implausibility
    3. Too much power (>=5kW)
    */
    String shutdownError = "ar2:kill:";
    Serial1.println(shutdownError + errCode);//Send to arduino first
    Serial.println(shutdownError + errCode);//Now send to computer
    //todo should error code be always in same address?
    EEPROM.write(0,(char)errCode);
}
