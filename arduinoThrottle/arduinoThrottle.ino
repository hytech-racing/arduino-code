/*
Arduino 3
Type: Uno
Use: Read and process pedal values
*/

//Extended pot is low resistance
//todo set these pins to which pins are being used
//Pot1 and Pot2 used for acceleration
int pot1 = 0;
int pot2 = 1;
int pot3 = 2;//Pot3 used for brake
int pwmTorque = 5;

//todo change these values to actual values once installed
/*
It seems better to find the Arduino mapped values instead of converting into mV
every time the loop runs
Extended = more resistance, Pedal compresses pot when pressed, therefore higher voltage reading corresponds to higher torque
*/
//Pot1 and Pot2 used for acceleration
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

void setup() {
    pinMode(pwmTorque, OUTPUT);
}

void loop() {
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

    if (potAccAdjDiff > 300) {//Acceleration error check (Die if 30%+ difference between adjusted values)
        analogWrite(pwmTorque, 0);
    } else {
        if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
            brakePlausActive = true;
            analogWrite(pwmTorque, 0);
        } else {
            if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                brakePlausActive = false;
            }
            if (!brakePlausActive) {//If brake plausibility is not active
                analogWrite(pwmTorque, torqueValAdjusted);
            } else {//If brake plausibility is active
                analogWrite(pwmTorque, 0);
            }
        }
    }
}
