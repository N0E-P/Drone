//librairies
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <Wire.h>  // Wire library - used for I2C communication

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

//create "Servo" objects to control the ESC
Servo motor1;
Servo motor2;
Servo motor3;
Servo motor4;

//Time units
unsigned long lastReceiveTime = 0;
unsigned long lastBipTime = 0;

//Declare the motor values variables
int motor1Value = 0;
int motor2Value = 0;
int motor3Value = 0;
int motor4Value = 0;

//Pitch, Yaw and Roll are more sensible if this value go up
const int calibration = 10;
const int accelerometerCalibration = 30;

// vehicule tension value
float tension = 0;

// Accelerometer
int ADXL345 = 0x53;         // The ADXL345 sensor I2C address
float X_out, Y_out, Z_out;  // Outputs
float rollAngle, pitchAngle = 0;

///////////////////////////////////
void setup() {
  Serial.begin(9600);

  // Buzzer
  pinMode(7, OUTPUT);

  //radio
  radio.begin();
  radio.openWritingPipe(addresses[0]);     // 00001
  radio.openReadingPipe(1, addresses[1]);  // 00002
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);

  //Accelerometer
  Wire.begin();  // Initiate the Wire library
  //Set ADXL345 in measuring mode
  Wire.beginTransmission(ADXL345);  // Start communicating with the device
  Wire.write(0x2D);                 // Access/ talk to POWER_CTL Register - 0x2D
  // Enable measurement
  Wire.write(8);  // (8dec -> 0000 1000 binary) Bit D3 High for measuring enable
  Wire.endTransmission();
  delay(10);

  // Off-set Calibration
  //X-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1E);  // X-axis offset register
  Wire.write(-2);
  Wire.endTransmission();
  delay(10);

  //Y-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1F);  // Y-axis offset register
  Wire.write(-0);
  Wire.endTransmission();
  delay(10);

  //Z-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x20);  // Z-axis offset register
  Wire.write(+1);
  Wire.endTransmission();
  delay(10);

  //Give each motor an outpout pin
  motor1.attach(5);
  motor2.attach(4);
  motor3.attach(3);
  motor4.attach(2);
  delay(2000);
}


///////////////////////////////////
void loop() {
  // Mesure and send battery tension
  tension = analogRead(A6) * 0.0244140625 + 0.5;
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
    lastBipTime = millis();
    Serial.print("     ");

    // Bip every 5 seconds if we lost signal
  } else {
    Serial.print("  X  ");
    if (millis() - lastBipTime > 5000) {
      tone(7, 2000);
      delay(75);
      tone(7, 3000);
      delay(75);
      tone(7, 4000);
      delay(75);
      noTone(7);
      lastBipTime = millis();
    }
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
    Wire.write(0x32);  // Start with register 0x32 (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(ADXL345, 6, true);  // Read 6 registers total, each axis value is stored in 2 registers

    // Get the values
    X_out = (Wire.read() | Wire.read() << 8);  // X-axis value
    X_out = X_out / 256;                       //For a range of +-2g, we need to divide the raw values by 256, according to the datasheet
    Y_out = (Wire.read() | Wire.read() << 8);  // Y-axis value
    Y_out = Y_out / 256;
    Z_out = (Wire.read() | Wire.read() << 8);  // Z-axis value
    Z_out = Z_out / 256;

    // Calculate Roll and Pitch (rotation around X-axis, rotation around Y-axis)
    rollAngle = atan(Y_out / sqrt(pow(X_out, 2) + pow(Z_out, 2))) * 180 / PI;
    pitchAngle = atan(-1 * X_out / sqrt(pow(Y_out, 2) + pow(Z_out, 2))) * 180 / PI;

    Serial.print("roll: ");
    Serial.print(rollAngle);
    Serial.print("  pitch: ");
    Serial.print(pitchAngle);
    Serial.print("     ");

    int throttle = map(data.throttle, 0, 255, 90, 255);

    // If radio signal lost, decrease the drone altitude
    if (millis() - lastReceiveTime > 1000) {
      motor1Value = 120;
      motor2Value = 120;
      motor3Value = 120;
      motor4Value = 120;


      // Stabilized mode
    } else if (!data.fast) {
      // drone tilts frontwards
      if (pitchAngle < 0) {
        int pitch = map(pitchAngle, 0, -90, 0, accelerometerCalibration);
        motor1Value = throttle + pitch;
        motor2Value = throttle + pitch;
        motor3Value = throttle - pitch;
        motor4Value = throttle - pitch;

        // drone tilts backwards
      } else {
        int pitch = map(pitchAngle, 0, 90, 0, accelerometerCalibration);
        motor1Value = throttle - pitch;
        motor2Value = throttle - pitch;
        motor3Value = throttle + pitch;
        motor4Value = throttle + pitch;
      }

      // drone tilts right
      if (pitchAngle < 0) {
        int roll = map(rollAngle, 0, -90, 0, accelerometerCalibration);
        motor1Value = motor1Value + roll;
        motor2Value = motor2Value - roll;
        motor3Value = motor3Value - roll;
        motor4Value = motor4Value + roll;

        // drone tilts left
      } else {
        int roll = map(rollAngle, 0, 90, 0, accelerometerCalibration);
        motor1Value = motor1Value - roll;
        motor2Value = motor2Value + roll;
        motor3Value = motor3Value + roll;
        motor4Value = motor4Value - roll;
      }


      // Normal flight mode
    } else {
      //Pitch front + Throttle
      if (data.pitch > 127) {
        int pitch = map(data.pitch, 128, 255, 0, calibration);
        motor1Value = throttle - pitch;
        motor2Value = throttle - pitch;
        motor3Value = throttle + pitch;
        motor4Value = throttle + pitch;

        //Pitch back + Throttle
      } else {
        int pitch = map(data.pitch, 127, 0, 0, calibration);
        motor1Value = throttle + pitch;
        motor2Value = throttle + pitch;
        motor3Value = throttle - pitch;
        motor4Value = throttle - pitch;
      }

      //Roll right
      if (data.roll > 127) {
        int roll = map(data.roll, 128, 255, 0, calibration);
        motor1Value = motor1Value - roll;
        motor2Value = motor2Value + roll;
        motor3Value = motor3Value + roll;
        motor4Value = motor4Value - roll;

        //Roll left
      } else {
        int roll = map(data.roll, 127, 0, 0, calibration);
        motor1Value = motor1Value + roll;
        motor2Value = motor2Value - roll;
        motor3Value = motor3Value - roll;
        motor4Value = motor4Value + roll;
      }

      //Yaw right
      if (data.yaw > 127) {
        int yaw = map(data.yaw, 128, 255, 0, calibration);
        motor1Value = motor1Value - yaw;
        motor2Value = motor2Value + yaw;
        motor3Value = motor3Value - yaw;
        motor4Value = motor4Value + yaw;

        //Yaw left
      } else {
        int yaw = map(data.yaw, 127, 0, 0, calibration);
        motor1Value = motor1Value + yaw;
        motor2Value = motor2Value - yaw;
        motor3Value = motor3Value + yaw;
        motor4Value = motor4Value - yaw;
      }
    }


    // Do not send negative values or values over the maximum throttle possible (max = 255)
    motor1Value = constrain(motor1Value, 0, 255);
    motor2Value = constrain(motor2Value, 0, 255);
    motor3Value = constrain(motor3Value, 0, 255);
    motor4Value = constrain(motor4Value, 0, 255);

    // Send data to the motors
    motor1.writeMicroseconds(map(motor1Value, 0, 255, 1100, 2100));
    motor2.writeMicroseconds(map(motor2Value, 0, 255, 1100, 2100));
    motor3.writeMicroseconds(map(motor3Value, 0, 255, 1100, 2100));
    motor4.writeMicroseconds(map(motor4Value, 0, 255, 1100, 2100));

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


  delay(50);
}
