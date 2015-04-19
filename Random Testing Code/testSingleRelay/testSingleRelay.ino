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
unsigned long runLoop;
int digitalBrake = 48;

String inputCmdStream1 = "";
boolean stringComplete1 = false;
String inputCmd1;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);
  
  inputCmdStream1.reserve(50);
  
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(46, OUTPUT);//trans 4
  pinMode(48, OUTPUT);//trans 3
  digitalWrite(48, LOW);
  pinMode(52, OUTPUT);//trans 1
  runLoop = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    int i = Serial.parseInt();
    if (i == 0) {
      digitalWrite(2, HIGH);
      digitalWrite(3, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
      digitalWrite(7, HIGH);
      digitalWrite(8, HIGH);
      digitalWrite(9, HIGH);
      Serial.println("Relays reset");
    }
    else {
      digitalWrite(i, LOW);
      Serial.print(i);
      Serial.println(" closed");
    }
    Serial1.println("ar2:throttle:1");
    Serial2.println("ar2:throttle:2");
    Serial3.println("ar2:throttle:3");
  }
  if (runLoop < millis()) {
    runLoop += 200;
    
    if (stringComplete1) {
      Serial.println(inputCmdStream1);
      inputCmdStream1 = "";
    }
    
    
    /******************************
    Throttle reading code
    ******************************/
    //Read analog values all at once
    pot1ValAdjusted = analogRead(pot1) + analogRead(pot1) + analogRead(pot1);
    pot2ValAdjusted = analogRead(pot2) + analogRead(pot2) + analogRead(pot2);
    pot3ValAdjusted = analogRead(pot3) + analogRead(pot3) + analogRead(pot3);
    pot1ValAdjusted /= 3;
    pot2ValAdjusted /= 3;
    pot3ValAdjusted /= 3;
    Serial.print("Raw throttle readings: ");
    Serial.print(pot1ValAdjusted);
    Serial.print(" ");
    Serial.print(pot2ValAdjusted);
    Serial.print(" ");
    Serial.println(pot3ValAdjusted);

    if (pot1ValAdjusted > 1000 || pot1ValAdjusted < 10 || pot2ValAdjusted > 1000 || pot2ValAdjusted < 10) {
        Serial.println("Throttle encoder short detected");
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
    Serial.print("Throttle value ");
    Serial.println(torqueValAdjusted);//Prints torque value to computer

    if (pot3ValAdjusted > 0) { //Brake light
        digitalWrite(digitalBrake, HIGH);
        Serial.println("Brake ON");
    } else if (pot3ValAdjusted <= 0) {
        digitalWrite(digitalBrake, LOW);
        Serial.println("Brake OFF");
    }

    if (potAccAdjDiff > 200) {//Acceleration error check (Die if 20%+ difference between readings)
        //todo does this need to shut down car or just send 0 torque val?
        //todo put error on screen
        Serial.println("Acceleration Implausibility on");
    } else {
        //todo remove error from screen
        Serial.println("Acceleration Implausibility off");
        if (pot3ValAdjusted > 0 && torqueVal >= 250) {//If brake pressed and torque pressed over 25%
            brakePlausActive = true;
            //todo put error on screen
            
            Serial.println("Brake plausibility on");
        } else if (brakePlausActive && torqueVal < 50) {//Motor deactivated but torque less than 5% (required before disabling brake plausibility)
            brakePlausActive = false;
            //todo remove error from screen
            Serial.println("Brake plausibility off");
        }
        if (brakePlausActive) {//If brake plausibility is active
            //todo put 0 throttle on screen
        } else {//If brake plausibility is not active
            //todo put throttle on screen
            Serial1.print("ar2:throttle:");
            Serial1.print(torqueVal);
            Serial2.print("ar2:throttle:");
            Serial2.print(torqueVal);
            Serial3.print("ar2:throttle:");
            Serial3.print(torqueVal);
        }
    }
    /******************************
    End throttle reading code
    ******************************/     
  }
}

void SerialEvent2() {
    while (Serial2.available()) {
        char newChar = (char)Serial2.read();
        if (newChar == '\n') {
            stringComplete1 = true;
        } else {
            inputCmdStream1 += newChar;
        }
    }
}
