int j = 0;
char g = 0;
void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.begin(9600);
  Serial2.begin(115200);
  for(j = 2; j < 10; j++) {
   digitalWrite(j, HIGH);
  }
  
}

void loop() {
  if(Serial.available()) {
    g = Serial.read();
  }
  switch g {
    case "A" :
      digitalWrite(2, LOW);
      break;
    case "S":
    digitalWrite(3, LOW);
      break;
    case "D":
    digitalWrite(4, LOW);
      break;
    case "F":
    digitalWrite(5, LOW);
      break;
    case "G":
    digitalWrite(6, LOW);
      break;
    case "H":
    digitalWrite(7, LOW);
      break;
    case "J":
    digitalWrite(8, LOW);
      break;
    case "K":
    digitalWrite(9, LOW);
      break;
  }
  delay(2000);
  /*
  for(j = 2; j < 10; j++) {
   
  
    
   
  delay(1000);
  
   digitalWrite(j, LOW);
   Serial.print("relay ");
  Serial.println(j-1);
  Serial2.println("ar2:throttle:1034");
  delay(5000);
  digitalWrite(j, HIGH);
 */
  }

}
