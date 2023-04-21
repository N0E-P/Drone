//librairies
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//radio
RF24 radio(8, 9);  // CE, CSN
const byte addresses[][6] = { "00001", "00002" };

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

// vehicule tension value
float tension = 0.00;

//Time units
unsigned long lastSignalBipTime = 0;
unsigned long lastTensionBipTime = 0;


///////////////////////////////////
void setup() {
  Serial.begin(9600);

  // Buzzer
  pinMode(3, OUTPUT);

  //radio communication
  radio.begin();
  radio.openWritingPipe(addresses[1]);     // 00002
  radio.openReadingPipe(1, addresses[0]);  // 00001
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);

  // Make sure the vehicule is stopped before starting
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  data.startAndStop = 0;
}


///////////////////////////////////
void loop() {
  //Check if we push the start and stop button
  if (!digitalRead(2)) {
    data.startAndStop = !data.startAndStop;
    delay(200);
  }

  //Store null entries if we stopped the motors
  if (!data.startAndStop) {
    data.throttle = 0;
    data.yaw = 127;
    data.pitch = 127;
    data.roll = 127;
    Serial.println("Motors Stopped.");


    //Read analog entries, convert them between 0 & 255, then store it in the data struct
  } else {
    data.throttle = map(analogRead(A2), 0, 1023, 255, 0);
    data.yaw = map(analogRead(A3), 0, 1023, 255, 0);
    data.pitch = map(analogRead(A0), 0, 1023, 255, 0);
    data.roll = map(analogRead(A1), 0, 1023, 255, 0);

    //Print values in the serial monitor
    Serial.print("Throttle: ");
    Serial.print(data.throttle);
    Serial.print("   Yaw: ");
    Serial.print(data.yaw);
    Serial.print("   Pitch: ");
    Serial.print(data.pitch);
    Serial.print("   Roll: ");
    Serial.println(data.roll);
  }


  //Print tension data
  Serial.print(tension);
  Serial.print("V");


  //Send the data
  radio.stopListening();
  radio.write(&data, sizeof(Data_Package));
  delay(5);


  // Receive data
  radio.startListening();
  if (radio.available()) {
    radio.read(&tension, sizeof(tension));
    lastSignalBipTime = millis();
    if (tension > 6.6) {
      Serial.print("     ");


      // Bip every 5s if the battery is low
    } else {
      Serial.print("  B  ");
      if (millis() - lastTensionBipTime > 5000) {
        tone(3, 800);
        delay(75);
        tone(3, 600);
        delay(75);
        noTone(3);
        lastTensionBipTime = millis();
      }
    }


    // Bip every 2 seconds if we lost signal
  } else {
    Serial.print("  X  ");
    if (millis() - lastSignalBipTime > 2000) {
      tone(3, 2000);
      delay(75);
      tone(3, 3000);
      delay(75);
      tone(3, 4000);
      delay(75);
      noTone(3);
      lastSignalBipTime = millis();
    }
  }


  delay(50);
}