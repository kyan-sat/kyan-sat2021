#define RIN1 8
#define RIN2 7
#define LIN1 6
#define LIN2 5
void setup() {
  pinMode(RIN1, OUTPUT);
  pinMode(RIN2, OUTPUT);
  pinMode(LIN1, OUTPUT);
  pinMode(LIN2, OUTPUT);
  Serial.begin(9600);
}
void loop() {
  Serial.println("R+ L+");
  analogWrite(RIN1, 255);
  analogWrite(RIN2, 0);
  analogWrite(LIN1, 255);
  analogWrite(LIN2, 0);
  delay(3000);
  Serial.println("R- L-");
  analogWrite(RIN1, 0);
  analogWrite(RIN2, 255);
  analogWrite(LIN1, 0);
  analogWrite(LIN2, 255);
  delay(3000);
  Serial.println("R+ L-");
  analogWrite(RIN1, 255);
  analogWrite(RIN2, 0);
  analogWrite(LIN1, 0);
  analogWrite(LIN2, 255);
  delay(3000);
  Serial.println("R- L+");
  analogWrite(RIN1, 0);
  analogWrite(RIN2, 255);
  analogWrite(LIN1, 255);
  analogWrite(LIN2, 0);
  delay(3000);
}
/*
void loop() {
  Serial.println("R+ L+");
  digitalWrite(RIN1, HIGH);
  digitalWrite(RIN2, LOW);
  digitalWrite(LIN1, HIGH);
  digitalWrite(LIN2, LOW);
  delay(3000);
  Serial.println("R- L-");
  digitalWrite(RIN1, LOW);
  digitalWrite(RIN2, HIGH);
  digitalWrite(LIN1, LOW);
  digitalWrite(LIN2, HIGH);
  delay(3000);
  Serial.println("R+ L-");
  digitalWrite(RIN1, HIGH);
  digitalWrite(RIN2, LOW);
  digitalWrite(LIN1, LOW);
  digitalWrite(LIN2, HIGH);
  delay(3000);
  Serial.println("R- L+");
  digitalWrite(RIN1, LOW);
  digitalWrite(RIN2, HIGH);
  digitalWrite(LIN1, HIGH);
  digitalWrite(LIN2, LOW);
  delay(3000);
}
*/
