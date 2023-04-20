//librairies
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

//radio
RF24 radio(8, 9);  // CE, CSN
const byte address[6] = "00001";

//Struct used to store all the data we want to send
struct Data_Package {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  boolean startAndStop;
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
unsigned long currentTime = 0;

//Declare the motor values variables
int motor1Value = 0;
int motor2Value = 0;
int motor3Value = 0;
int motor4Value = 0;

//Pitch, Yaw and Roll can each interfer on the Throttle by this value
const int calibration = 50;


void setup() {
  Serial.begin(9600);

  //radio
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  //Give each motor an outpout pin
  motor1.attach(5);
  motor2.attach(4);
  motor3.attach(3);
  motor4.attach(2);

  delay(5000);
}


void loop() {
float tension =  analogRead(A5)  * 0.0244140625;
  Serial.print(tension);

  //Read and store the struct send by the radio
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package));
    lastReceiveTime = millis();
  }

  // Stop the motors
  if (data.startAndStop == false) {
    motor1.writeMicroseconds(1100);
    motor2.writeMicroseconds(1100);
    motor3.writeMicroseconds(1100);
    motor4.writeMicroseconds(1100);
    Serial.println("Motors Stopped.");
    delay(50);
    return;
  }

  //Slow the motors if we lost signal for more than a second
  currentTime = millis();
  if (currentTime - lastReceiveTime > 1000) {
    motor1.writeMicroseconds(1300);
    motor2.writeMicroseconds(1300);
    motor3.writeMicroseconds(1300);
    motor4.writeMicroseconds(1300);
    Serial.println("Signal lost.");
    delay(50);
    return;
  }


  //Pitch front + Throttle
  if (data.pitch > 127) {
    data.pitch = map(data.pitch, 128, 255, 0, calibration);
    motor1Value = data.throttle - data.pitch;
    motor2Value = data.throttle - data.pitch;
    motor3Value = data.throttle + data.pitch;
    motor4Value = data.throttle + data.pitch;

    //Pitch back + Throttle
  } else {
    data.pitch = map(data.pitch, 127, 0, 0, calibration);
    motor1Value = data.throttle + data.pitch;
    motor2Value = data.throttle + data.pitch;
    motor3Value = data.throttle - data.pitch;
    motor4Value = data.throttle - data.pitch;
  }

  //Roll right
  if (data.roll > 127) {
    data.roll = map(data.roll, 128, 255, 0, calibration);
    motor1Value = motor1Value - data.roll;
    motor2Value = motor2Value + data.roll;
    motor3Value = motor3Value + data.roll;
    motor4Value = motor4Value - data.roll;

    //Roll left
  } else {
    data.roll = map(data.roll, 127, 0, 0, calibration);
    motor1Value = motor1Value + data.roll;
    motor2Value = motor2Value - data.roll;
    motor3Value = motor3Value - data.roll;
    motor4Value = motor4Value + data.roll;
  }

  //Yaw right
  if (data.yaw > 127) {
    data.yaw = map(data.yaw, 128, 255, 0, calibration);
    motor1Value = motor1Value - data.yaw;
    motor2Value = motor2Value + data.yaw;
    motor3Value = motor3Value - data.yaw;
    motor4Value = motor4Value + data.yaw;

    //Yaw left
  } else {
    data.yaw = map(data.yaw, 127, 0, 0, calibration);
    motor1Value = motor1Value + data.yaw;
    motor2Value = motor2Value - data.yaw;
    motor3Value = motor3Value + data.yaw;
    motor4Value = motor4Value - data.yaw;
  }


  // Do not send negative values
  if (motor1Value < 0) {
    motor1Value = 0;
  }
  if (motor2Value < 0) {
    motor2Value = 0;
  }
  if (motor3Value < 0) {
    motor3Value = 0;
  }
  if (motor4Value < 0) {
    motor4Value = 0;
  }

  // Do not send over 255 values
  if (motor1Value > 255) {
    motor1Value = 255;
  }
  if (motor2Value > 255) {
    motor2Value = 255;
  }
  if (motor3Value > 255) {
    motor3Value = 255;
  }
  if (motor4Value > 255) {
    motor4Value = 255;
  }


  // Send data to the motors
  motor1.writeMicroseconds(map(motor1Value, 0, 255, 1100, 2100));
  motor2.writeMicroseconds(map(motor2Value, 0, 255, 1100, 2100));
  motor3.writeMicroseconds(map(motor3Value, 0, 255, 1100, 2100));
  motor4.writeMicroseconds(map(motor4Value, 0, 255, 1100, 2100));


  //


  //Write data on the Serial Monitor
  Serial.print("Throttle: ");
  Serial.print(data.throttle);
  Serial.print("   Yaw: ");
  Serial.print(data.yaw);
  Serial.print("   Pitch: ");
  Serial.print(data.pitch);
  Serial.print("   Roll: ");
  Serial.print(data.roll);
  Serial.print("   Motor1: ");
  Serial.print(motor1Value);
  Serial.print("   Motor2: ");
  Serial.print(motor2Value);
  Serial.print("   Motor3: ");
  Serial.print(motor3Value);
  Serial.print("   Motor4: ");
  Serial.println(motor4Value);

  delay(50);
}
