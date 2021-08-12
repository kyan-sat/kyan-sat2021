// https://deviceplus.jp/hobby/entry016/
#define echoPin A0  // Echo Pin
#define trigPin A1  // Trigger Pin

double duration;
double distance;

void setup() {
  Serial.begin(9600);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
}
void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  if (duration > 0) {
    duration /= 2;
    distance = duration * 340 * 100 / 1000000;
    Serial.print("distance:");
    Serial.print(distance);
    Serial.println(" cm");
  }
  delay(500);
}
