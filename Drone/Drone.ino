//librairies
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <Wire.h>

//radio
RF24 radio(8, 9);  // CE, CSN
const byte addresses[][6] = { "00001", "00002" };

//Struct used to store all the data we want to send
struct Data_Package {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  boolean start;
  boolean fast;
};

//Create a variable from the struct above
Data_Package data;

//Time units
unsigned long lastReceiveTime = 0;

// vehicule tension value
float tension = 0;

// Accelerometer
int ADXL345 = 0x53;
float X_out, Y_out, Z_out;
float rollAngle, pitchAngle = 0;
int accelerometerCalibration = 0;

//create "Servo" objects to control the ESC
Servo motor1;
Servo motor2;
Servo motor3;
Servo motor4;

// Values to send to the motors
int motor1Value, motor2Value, motor3Value, motor4Value = 0;
int pitch, yaw, roll, throttle = 0;

//Pitch, Yaw and Roll are more sensible if theses values go up
const int userCalibration = 75;


///////////////////////////////////
void setup() {
  Serial.begin(9600);

  //radio
  radio.begin();
  radio.openWritingPipe(addresses[0]);     // 00001
  radio.openReadingPipe(1, addresses[1]);  // 00002
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);

  //Accelerometer
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

  //Give each motor an outpout pin
  motor1.attach(5);
  motor2.attach(4);
  motor3.attach(3);
  motor4.attach(2);
  delay(1500);
}


///////////////////////////////////
void loop() {
  // Mesure and send battery tension
  tension = analogRead(A6) * 0.0244140625;
  Serial.print(tension);
  Serial.print("V");

  // Send the drone tension to the controler
  radio.stopListening();
  radio.write(&tension, sizeof(tension));
  delay(5);


  //Read the struct send by the radio
  radio.startListening();
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package));
    lastReceiveTime = millis();
    Serial.print("     ");

    // No signal
  } else {
    Serial.print("  X  ");
  }


  // Stop the motors
  if (!data.start) {
    motor1.writeMicroseconds(1100);
    motor2.writeMicroseconds(1100);
    motor3.writeMicroseconds(1100);
    motor4.writeMicroseconds(1100);
    Serial.println("Motors Stopped.");


    // Motors are rotating
  } else {
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

    // Life Telephone
    Serial.print("roll: ");
    Serial.print(rollAngle);
    Serial.print("  pitch: ");
    Serial.print(pitchAngle);
    Serial.print("  Zaxis: ");
    Serial.print(Z_out);


    // Shut down the motors if the drone is at more than 60° from flat
    if (Z_out < 0.5) {
      motor1Value = 1100;
      motor2Value = 1100;
      motor3Value = 1100;
      motor4Value = 1100;
      Serial.print("  F  ");


      // Otherwise, stabilize it
    } else {
      Serial.print("     ");

      // If radio signal lost, decrease the drone altitude
      if (millis() - lastReceiveTime > 1000) {
        throttle = 1500;

        // Otherwise, get the controler throttle value
      } else {
        throttle = map(data.throttle, 0, 255, 1400, 2100);
      }

      // reduce the calibration effect at high speed
      accelerometerCalibration = map(throttle, 1400, 2100, 175, 75);

      // drone tilts frontwards
      if (pitchAngle < 0) {
        pitch = map(pitchAngle, 0, -90, 0, accelerometerCalibration);
        motor1Value = throttle + pitch;
        motor2Value = throttle + pitch;
        motor3Value = throttle - pitch;
        motor4Value = throttle - pitch;

        // drone tilts backwards
      } else {
        pitch = map(pitchAngle, 0, 90, 0, accelerometerCalibration);
        motor1Value = throttle - pitch;
        motor2Value = throttle - pitch;
        motor3Value = throttle + pitch;
        motor4Value = throttle + pitch;
      }

      // drone tilts right
      if (pitchAngle < 0) {
        roll = map(rollAngle, 0, -90, 0, accelerometerCalibration);
        motor1Value = motor1Value + roll;
        motor2Value = motor2Value - roll;
        motor3Value = motor3Value - roll;
        motor4Value = motor4Value + roll;

        // drone tilts left
      } else {
        roll = map(rollAngle, 0, 90, 0, accelerometerCalibration);
        motor1Value = motor1Value - roll;
        motor2Value = motor2Value + roll;
        motor3Value = motor3Value + roll;
        motor4Value = motor4Value - roll;
      }
    }


    // Use values from controller if got connexion
    if (millis() - lastReceiveTime < 1000) {
      //Pitch front
      if (data.pitch > 127) {
        pitch = map(data.pitch, 128, 255, 0, userCalibration);
        motor1Value = motor1Value - pitch;
        motor2Value = motor2Value - pitch;
        motor3Value = motor3Value + pitch;
        motor4Value = motor4Value + pitch;

        //Pitch back
      } else {
        pitch = map(data.pitch, 127, 0, 0, userCalibration);
        motor1Value = motor1Value + pitch;
        motor2Value = motor2Value + pitch;
        motor3Value = motor3Value - pitch;
        motor4Value = motor4Value - pitch;
      }

      //Roll right
      if (data.roll > 127) {
        roll = map(data.roll, 128, 255, 0, userCalibration);
        motor1Value = motor1Value - roll;
        motor2Value = motor2Value + roll;
        motor3Value = motor3Value + roll;
        motor4Value = motor4Value - roll;

        //Roll left
      } else {
        roll = map(data.roll, 127, 0, 0, userCalibration);
        motor1Value = motor1Value + roll;
        motor2Value = motor2Value - roll;
        motor3Value = motor3Value - roll;
        motor4Value = motor4Value + roll;
      }

      //Yaw right
      if (data.yaw > 127) {
        yaw = map(data.yaw, 128, 255, 0, userCalibration);
        motor1Value = motor1Value - yaw;
        motor2Value = motor2Value + yaw;
        motor3Value = motor3Value - yaw;
        motor4Value = motor4Value + yaw;

        //Yaw left
      } else {
        yaw = map(data.yaw, 127, 0, 0, userCalibration);
        motor1Value = motor1Value + yaw;
        motor2Value = motor2Value - yaw;
        motor3Value = motor3Value + yaw;
        motor4Value = motor4Value - yaw;
      }
    }


    // To control the motors, only send values between 1100 and 2100
    motor1Value = constrain(motor1Value, 1100, 2100);
    motor2Value = constrain(motor2Value, 1100, 2100);
    motor3Value = constrain(motor3Value, 1100, 2100);
    motor4Value = constrain(motor4Value, 1100, 2100);

    // Send data to the motors
    motor1.writeMicroseconds(motor1Value);
    motor2.writeMicroseconds(motor2Value);
    motor3.writeMicroseconds(motor3Value);
    motor4.writeMicroseconds(motor4Value);

    //Write data on the Serial Monitor
    Serial.print("Throttle: ");
    Serial.print(data.throttle);
    Serial.print("   Yaw: ");
    Serial.print(data.yaw);
    Serial.print("   Pitch: ");
    Serial.print(data.pitch);
    Serial.print("   Roll: ");
    Serial.print(data.roll);
    Serial.print("     Motor1: ");
    Serial.print(motor1Value);
    Serial.print("   Motor2: ");
    Serial.print(motor2Value);
    Serial.print("   Motor3: ");
    Serial.print(motor3Value);
    Serial.print("   Motor4: ");
    Serial.println(motor4Value);
  }


  delay(10);
}
