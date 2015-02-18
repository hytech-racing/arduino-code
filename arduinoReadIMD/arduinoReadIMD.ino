

// IMD has two outputs: PWM on pin 6 (Mls, i think), and on/off on pin 8 (OHhs). voltage bridge is needed to bring voltage down to measureable levels.
long highPulse = 0;
long lowPulse = 0;
long totalPulse = 0;
int pulseVal = 0;
int voltageSig = 0;
long lastSignal = 0;

void setup() {
  Serial.begin(9600);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
}
void loop() {
  
  voltageSig = 0;
  highPulse = pulseIn(7, HIGH, 1500000);
  lowPulse = pulseIn(7, LOW, 1500000);
  totalPulse = highPulse + lowPulse;
  pulseVal = map(highPulse, 0, totalPulse, 0, 100);
  if (digitalRead(6) == HIGH) {
    voltageSig = 1;
  }
  
  Serial.print(highPulse);
  Serial.write(32);
  Serial.print(lowPulse);
  Serial.write(32);
  Serial.print(pulseVal);
  Serial.write(32);
  Serial.println(voltageSig);
  
  
}
