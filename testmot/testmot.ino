#include <Servo.h>
Servo moteur;

int motorValue;

void setup() {
  Serial.begin(9600);

  moteur.attach(9);
}

void loop() {
  motorValue = analogRead(A0);
  motorValue = map(motorValue, 0, 1023, 1100, 2100);
  moteur.writeMicroseconds(motorValue);

  Serial.println(motorValue);
}