int pin = A6;

double raw_voltage;

void setup() {
  Serial.begin(9600);
  pinMode(pin, INPUT);
}

void loop() {
  raw_voltage = analogRead(pin);
  Serial.print("raw value: ");
  Serial.println(raw_voltage);
  delay(100);
}
