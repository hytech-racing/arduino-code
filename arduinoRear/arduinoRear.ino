/*
Arduino 1
Type: Mega
Use: Main show
*/

/*************************************
BEGIN CONFIGURATION
*************************************/
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


int digitalBrake = 46;
/*************************************
END CONFIGURATION
*************************************/


int rpmVal = 0; // not sure if we will read in the values for RPM or not

void setup() {
    pinMode(digitalBrake, OUTPUT);
    Serial.begin(115200);
}

void loop() {

    digitalWrite(digitalBrake, HIGH);
    delay(1000);
    digitalWrite(digitalBrake, LOW);
    delay(1000);
}
