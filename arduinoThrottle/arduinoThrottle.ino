/*
Arduino 3
Type: Uno
Use: Read and process pedal values
*/

int torqueOut = 10;//Torque value sent to digital potentiometer

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
int pot1High = 650;//Pedal pressed
int pot1Low = 275;//Pedal resting//todo right now the low vals are when a little pressure is applied
int pot2High = 650;
int pot2Low = 275;
int pot3High = 689;//Pot3 used for brake
int pot3Low = 210;

float pot1ValAdjusted;
float pot2ValAdjusted;
float pot3ValAdjusted;
float potAccAdjDiff;//Holds the difference between two accelerator readings
float pot1Range = pot1High - pot1Low;//Ranges that each pot will move (used for percentage calcs)
float pot2Range = pot2High - pot2Low;
float pot3Range = pot3High - pot3Low;

float torqueVal;//0-1000 mapped value for torque
float torqueValAdjusted;//0-255 adjusted exponentially

boolean regenActive = false;

boolean brakePlausActive = false;//Set to true if brakes actuated && torque encoder > 25%

unsigned long runLoop;//Stores millisecond value to run main loop every so often (instead of using delay)

boolean ready2Drive = false;

String inputCmd = "";

void setup() {
  Serial.begin(115200);
  runLoop = 0;
  timeoutRx = 1000;
}

void loop() {
    if (stringComplete) {//Recieved something on Serial
        if (inputCmd == "ar3:ready2Drive") {
            ready2Drive = true;
        }else if (inputCmd == "ar3:restart") {
            //Restarting vehicle
            ready2Drive = false;
        }
        inputCmd = "";
        stringComplete = false;
    }
    if (runLoop < millis()) {//Runs whether or not car is ready to drive
        if (!ready2Drive) {
            runLoop = millis() + 100;//Push runLoop up 100 ms ONLY if car is not ready to drive (otherwise next top-level if loop will increment runLoop)
        }

        //Read analog values all at once
        pot1ValAdjusted = analogRead(pot1);
        pot2ValAdjusted = analogRead(pot2);
        pot3ValAdjusted = analogRead(pot3);

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
        analogWrite(10, torqueValAdjusted);
        Serial.print("ar2:throttle:");
        Serial.println(torqueValAdjusted);//Sends torque value to dash arduino
        Serial.print("ar1:print:Throttle value ");
        Serial.println(torqueValAdjusted);//Prints torque value to computer

        if (pot3ValAdjusted > 0) { //Brake light
            Serial.println("ar1:brake:1");
            Serial.println("ar1:print:Brake lights on");
        } else {
            Serial.println("ar1:brake:0");
            Serial.println("ar1:print:Brake lights off");
        }
    }
    if (runLoop < millis() && ready2Drive){//Runs only if car is ready to drive
        runLoop = millis() + 100;//Push runLoop up 100 ms

        if (potAccDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
            //todo error checking which can detect open circuit, short to ground and short to sensor power
            //todo does this need to shut down car or just send 0 torque val?
            Serial.println("ar2:accelImplaus:1");
            Serial.println("ar1:print:Acceleration Implausibility on")
            analogWrite(pwmTorque, 0);
        } else {
            Serial.println("ar2:accelImplaus:0")
            Serial.println("ar1:print:Acceleration Implausibility off")
            if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
                brakePlausActive = true;
                analogWrite(pwmTorque, 0);
                Serial.println("ar2:brakePlaus:1");
                Serial.println("ar1:print:Brake plausibility on");
            } else {
                if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                    brakePlausActive = false;
                    Serial.println("ar2:brakePlaus:0");
                    Serial.println("ar1:print:Brake plausibility off");
                }
                if (!brakePlausActive) {//If brake plausibility is not active
                    analogWrite(pwmTorque, torqueValAdjusted);
                } else {//If brake plausibility is active
                    analogWrite(pwmTorque, 0);
                }
            }
        }

    }
    if (timeoutRx < millis()) {//If 1 second has passed since receiving a complete command send shutdown command
        ready2Drive = false;
        Serial.println("ar1:restart");
        Serial.print("ar1:print:ar3 lost communication to ar2");
    }
}

void serialEvent() {//Receive bytes from dash arduino
  while (Serial.available()) {
    char newChar = (char)Serial.read();
    if (newChar == '\n') {//NOTE: inputCmd does NOT include \n
      stringComplete = true;
      timeoutRx = millis() + 1000; //Number of milliseconds since program started, plus 1000, used to timeout if no complete command received for 1 second
    }else {
      inputCmd += newChar;
    }
  }
}
