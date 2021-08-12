int pin = 11;

void setup() {
  pinMode(pin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(pin, LOW);
  Serial.println("low");
  delay(1000);
  digitalWrite(pin, HIGH);
  Serial.println("high");
  delay(1000);
}
