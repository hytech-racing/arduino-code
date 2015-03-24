/*
Arduino 3
Type: Uno (currently, for testing)
Use: Currently for testing other Arduinos
*/

int pwmBmsTorque = 7;//todo Not yet in wiring diagram


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
int pot1Low = 305;//Pedal resting//todo right now the low vals are when a little pressure is applied
int pot2High = 755;
int pot2Low = 413;
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

void setup() {
  Serial.begin(115200);
  runLoop = 0;
  pinMode(8, OUTPUT);
}

void loop() {
  if (runLoop < millis()){//Runs 1x per second
    runLoop = millis() + 200;//Push runLoop up 1000 ms

    //Read analog values all at once
    pot1ValAdjusted = analogRead(pot1);
    pot2ValAdjusted = analogRead(pot2);
    pot3ValAdjusted = analogRead(pot3);

    pot1ValAdjusted = pot1ValAdjusted - pot1Low;
    pot1ValAdjusted = pot1ValAdjusted * 1000;
    pot1ValAdjusted = pot1ValAdjusted / pot1Range;

    pot2ValAdjusted = pot2ValAdjusted - pot2Low;
    pot2ValAdjusted = pot2ValAdjusted * 1000;
    pot2ValAdjusted = pot2ValAdjusted / pot2Range;

    pot3ValAdjusted = pot3ValAdjusted - pot3Low;
    pot3ValAdjusted = pot3ValAdjusted * 1000;
    pot3ValAdjusted = pot3ValAdjusted / pot3Range;

    Serial.print("pot1 original val: ")
    Serial.println(analogRead(pot1));
    Serial.print("pot1ValAdjusted: ")
    Serial.println(pot1ValAdjusted);
    Serial.print("pot2 original val: ")
    Serial.println(analogRead(pot2));
    Serial.print("pot2ValAdjusted: ")
    Serial.println(pot2ValAdjusted);
    Serial.print("pot3 original val: ")
    Serial.println(analogRead(pot3));
    Serial.print("pot3ValAdjusted: ")
    Serial.println(pot3ValAdjusted);

    if(pot3ValAdjusted > 0) { //Brake light
        Serial.println("ar1:brake:1");
        digitalWrite(8, HIGH);
    }else {
        Serial.println("ar1:brake:0");
        digitalWrite(8, LOW);
    }

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
    Serial.print("Torque Value: ")
    Serial.println(torqueVal);
    String txt8 = "Adjusted torque (0-5): ";
    Serial.println(txt8 + torqueValAdjusted);
    if (potAccDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
        //todo error checking which can detect open circuit, short to ground and short to sensor power
        Serial.println("acceleration implausibility detected");
        analogWrite(pwmBmsTorque, 0);
    } else {
        if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
            brakePlausActive = true;
            analogWrite(pwmBmsTorque, 0);
            Serial.println("brake plausibility activated");
        } else {
            if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
                brakePlausActive = false;
                Serial.println("brake plausibility deactivated");
            }
            if (!brakePlausActive) {//If brake plausibility is not active
                analogWrite(pwmBmsTorque, torqueValAdjusted);
                Serial.println("sending torque value to motor controller");
            } else {//If brake plausibility is active
                analogWrite(pwmBmsTorque, 0);
                Serial.println("sending 0 to motor controller");
            }
        }
    }

    Serial.println("\n\n\n\n");


  }
}
