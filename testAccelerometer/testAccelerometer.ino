// Accelerometer variables
#include <Wire.h>
int ADXL345 = 0x53;
float X_out, Y_out, Z_out;
float rollAngle, pitchAngle = 0;

void setup() {
  Serial.begin(9600);

  //Accelerometer initialisation
  Wire.begin();
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D);
  Wire.write(8);
  Wire.endTransmission();
  delay(10);

  // X, Y & Z axis Calibration
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1E);
  Wire.write(-2);
  Wire.endTransmission();
  delay(10);
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1F);
  Wire.write(-0);
  Wire.endTransmission();
  delay(10);
  Wire.beginTransmission(ADXL345);
  Wire.write(0x20);
  Wire.write(+1);
  Wire.endTransmission();
  delay(10);
}

void loop() {
  // Read acceleromter data
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);

  // Get the X, Y & Z values
  X_out = (Wire.read() | Wire.read() << 8);
  X_out = X_out / 256;
  Y_out = (Wire.read() | Wire.read() << 8);
  Y_out = Y_out / 256;
  Z_out = (Wire.read() | Wire.read() << 8);
  Z_out = Z_out / 256;

  // Calculate Roll and Pitch (rotation around X-axis, rotation around Y-axis)
  rollAngle = atan(Y_out / sqrt(pow(X_out, 2) + pow(Z_out, 2))) * 180 / PI;
  pitchAngle = atan(-1 * X_out / sqrt(pow(Y_out, 2) + pow(Z_out, 2))) * 180 / PI;

  // Prints
  // Serial.print("X= ");
  // Serial.print(X_out);
  // Serial.print("   Y= ");
  // Serial.print(Y_out);
  // Serial.print("   Zaxis: ");
  // Serial.print(Z_out);
  // Serial.print("   roll: ");
  Serial.print(rollAngle);
  // Serial.print("   pitch: ");
  Serial.print(" ");
  Serial.println(pitchAngle);
}