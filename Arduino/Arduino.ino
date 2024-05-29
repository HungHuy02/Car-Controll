#include <NewPing.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <ArduinoJson.h>
SoftwareSerial bluetooth(9, 8);
#define IN1 3
#define IN2 5
#define IN3 6
#define IN4 11
#define DL_S 12
#define DR_S 13
#define UL_S A0
#define UR_S A1
#define echo A2
#define trig A3
#define servoPin 7
#define CONTROL_MODE 0
#define AUTO_LINE 1
#define AUTO_OBSTACLE 2
#define AUTO_FOLLOW 3
#define MAX_DISTANCE 200 // Max distance for obstacle detection
#define gocTruoc 72
NewPing sonar(trig, echo, MAX_DISTANCE);
Servo myServo;
char command;
char temp = 'S';
int Speed = 100;
int mode = CONTROL_MODE;
int distance = 100;
int distanceL = 100;
int distanceR = 100;
int Dleft_sensor;
int Dright_sensor;
int Uleft_sensor;
int Uright_sensor;
int run = 0;
String json;
String jsonTemp;
bool bluetoothOn = false;
void forward();
void back();
void left();
void right();

void setup() {
  Serial.begin(9600);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
   bluetooth.begin(9600);
  myServo.attach(servoPin);
  myServo.write(gocTruoc);
  pinMode(DR_S, INPUT);
  pinMode(DL_S, INPUT);
  pinMode(UR_S, INPUT);
  pinMode(UL_S, INPUT);
  Wire.begin(100);   
}

void loop() {
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    if (bluetooth.available() > 0) {
      bluetoothOn = true;
      command = bluetooth.read();
      Serial.println(command);
    }else {
      bluetoothOn = false;
    }
    if(command != temp) {
      Stop();
    }
   switch (command) {
      case 'X':
        myServo.detach();
        Speed = 150;
        mode = CONTROL_MODE;
        temp = 'X';
        break;
      case 'Y':
      myServo.detach();
        Speed = 100;
         temp = 'Y';
        mode = AUTO_LINE;
        break;
      case 'Z':
      myServo.attach(servoPin);
        Speed = 100;
        mode = AUTO_OBSTACLE;
         temp = 'Z';
        myServo.attach(servoPin);
        break;
      case 'T':
      myServo.attach(servoPin);
        Speed = 100;
        mode = AUTO_FOLLOW;
         temp = 'T';
        myServo.write(gocTruoc);
        break;
      case 'F':
      myServo.detach();
        Speed = 150;
        forward();
        temp = 'F';
        mode = CONTROL_MODE;
        break;
      case 'B':
      myServo.detach();
        Speed = 150;
        back();
        temp = 'B';
        mode = CONTROL_MODE;
        break;
      case 'L':
      myServo.detach();
        Speed = 150;
        left();
        temp = 'L';
        mode = CONTROL_MODE;
        break;
      case 'R':
      myServo.detach();
        Speed = 150;
        right();
        temp = 'R';
        mode = CONTROL_MODE;
        break;
      case 'S':
        Stop();
        temp = 'S';
        break;
    }
    if (mode == AUTO_LINE) {
    AutoLine();
  } else if (mode == AUTO_OBSTACLE) {
    AutoObstacle();
  } else if (mode == AUTO_FOLLOW) {
    AutoFollow();
  } 
}
void AutoLine() {
  DynamicJsonDocument jsonDoc(256);
  Dleft_sensor = digitalRead(DL_S);
  Dright_sensor = digitalRead(DR_S);
  if (Dleft_sensor == 0 && Dright_sensor == 1) {
    if (run != 3 ) {
      Stop();
      back();
      delay(100);
      Stop();
    }
    right();
    jsonDoc["Dl"] = Dleft_sensor;
    jsonDoc["Dr"] = Dright_sensor;
    serializeJson(jsonDoc, jsonTemp);
    json = jsonTemp;
    delay(200);
  } else if (Dleft_sensor == 1 && Dright_sensor == 0) {
    if (run != 4 ) {
      Stop();
      back();
      delay(100);
      Stop();
    }
    left();
    jsonDoc["Dl"] = Dleft_sensor;
    jsonDoc["Dr"] = Dright_sensor;
    serializeJson(jsonDoc, jsonTemp);
    json = jsonTemp;
    delay(200);
  } else {
    if (run != 1 ) {
      Stop();
      delay(100);
    }
    forward();
  }
  jsonTemp = "";
}
void AutoObstacle() {
  distance = readPing();
if (distance <= 15) {
    Stop();
    delay(100);
    back();
    delay(300);
    Stop();
    delay(100);
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["d"] = distance;
    distanceR = lookR();
    delay(200);
    distanceL = lookL();
    jsonDoc["dR"] = distanceR;
    jsonDoc["dL"] = distanceL;
    serializeJson(jsonDoc, json);
    delay(200);
    if (distanceR >= distanceL) {
      Stop();
      right();
      delay(1000);
      Stop();
    } else {
      Stop();
      left();
      delay(1000);
      Stop();
    }
  } else {
    forward();
  }
}
void AutoFollow() {
  DynamicJsonDocument jsonDoc(256);
  jsonTemp = "";
  distance = readPing();
  Uright_sensor = digitalRead(UR_S);
  Uleft_sensor = digitalRead(UL_S);
  if (Uleft_sensor != 0 && Uright_sensor != 0) {
    if (distance <= 8) {
      if (run != 2 ) {
        Stop();
        delay(100);
      }
      jsonDoc["d"] = distance;
      jsonDoc["Ur"] = Uright_sensor;
      jsonDoc["Ul"] = Uleft_sensor;
      serializeJson(jsonDoc, jsonTemp);
  json = jsonTemp;
      back();
    } else if (distance <= 25 && distance > 12) {
      if (run != 1 ) {
        Stop();
        delay(100);
      }
      jsonDoc["d"] = distance;
      jsonDoc["Ur"] = Uright_sensor;
      jsonDoc["Ul"] = Uleft_sensor;
      serializeJson(jsonDoc, jsonTemp);
      json = jsonTemp;
      forward();
    } else {
      Stop();
    }
  } else if (Uleft_sensor == 0 && Uright_sensor != 0) {
    if (run != 3 ) {
      Stop();
      delay(100);
    }
    jsonDoc["d"] = distance;
    jsonDoc["Ur"] = Uright_sensor;
    jsonDoc["Ul"] = Uleft_sensor;
    serializeJson(jsonDoc, jsonTemp);
    json = jsonTemp;
    right();
  } else if (Uleft_sensor != 0 && Uright_sensor == 0) {
    if (run != 4 ) {
      Stop();
      delay(100);
    }
    jsonDoc["d"] = distance;
    jsonDoc["Ur"] = Uright_sensor;
    jsonDoc["Ul"] = Uleft_sensor;
    serializeJson(jsonDoc, jsonTemp);
    json = jsonTemp;
    left();
  } else if (Uleft_sensor == 0 && Uright_sensor == 0) {
    Stop();
  }
  jsonTemp = "";
}

int readPing() {
  delay(70);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}

int lookR() {
  myServo.write(gocTruoc - 50);
  delay(250);
  int distance = readPing();
  delay(200);
  myServo.write(gocTruoc);
  return distance;
}

int lookL() {
  myServo.write(gocTruoc + 50);
  delay(250);
  int distance = readPing();
  delay(200);
  myServo.write(gocTruoc);
  return distance;
}

void left() {
  run = 4;
  analogWrite(IN1, Speed);
  analogWrite(IN3, Speed);
}
void right() {
  run = 3;
  analogWrite(IN2, Speed);
  analogWrite(IN4, Speed);
}
void back() {
  run = 2;
  analogWrite(IN3, Speed);
  analogWrite(IN2, Speed);
}

void forward() {
  run = 1;
  analogWrite(IN4, Speed);
  analogWrite(IN1, Speed);
}
void Stop() {
  run = 0;
  digitalWrite(IN1, 0);
  digitalWrite(IN2, 0);
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 0);
}

void receiveEvent(int howMany) {
  while (0 < Wire.available() && !bluetoothOn) {
    command = Wire.read();      
  }
}

void requestEvent() {
  if(!json.equals("")) {
    if(json.length() < 32) {
    for(int i = json.length() + 1; i <= 32; i++) {
      json += " ";
    }
   }
    Wire.write(json.c_str()); 
   json = "";
  }
  
}
