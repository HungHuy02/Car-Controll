#include <NewPing.h>
#include <Servo.h>
#include <SoftwareSerial.h>
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
#define servoPin 10
#define CONTROL_MODE 0
#define AUTO_LINE 1
#define AUTO_OBSTACLE 2
#define AUTO_FOLLOW 3
#define MAX_DISTANCE 200 // Max distance for obstacle detection
#define gocTruoc 72
NewPing sonar(trig, echo, MAX_DISTANCE);
Servo myServo;
char command;
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
}
void loop() {
  if (bluetooth.available() > 0) {
    command = bluetooth.read();
    Serial.println(command);
    Stop();
    switch (command) {
      case 'X':
        myServo.detach();
        Speed = 150;
        mode = CONTROL_MODE;
        break;
      case 'Y':
      myServo.detach();
        Speed = 100;
        mode = AUTO_LINE;
        break;
      case 'Z':
      myServo.attach(servoPin);
        Speed = 100;
        mode = AUTO_OBSTACLE;
        myServo.attach(servoPin);
        break;
      case 'T':
      myServo.attach(servoPin);
        Speed = 100;
        mode = AUTO_FOLLOW;
        myServo.write(gocTruoc);
        break;
      case 'F':
      myServo.detach();
        Speed = 150;
        forward();
        mode = CONTROL_MODE;
        break;
      case 'B':
      myServo.detach();
        Speed = 150;
        back();
        mode = CONTROL_MODE;
        break;
      case 'L':
      myServo.detach();
        Speed = 150;
        left();
        mode = CONTROL_MODE;
        break;
      case 'R':
      myServo.detach();
        Speed = 150;
        right();
        mode = CONTROL_MODE;
        break;
      case 'S':
        Stop();
        break;
    }
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
    delay(200);
  } else if (Dleft_sensor == 1 && Dright_sensor == 0) {
    if (run != 4 ) {
      Stop();
      back();
      delay(100);
      Stop();
    }
    left();
    delay(200);
  } else {
    if (run != 1 ) {
      Stop();
      delay(100);
    }
    forward();
  }
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
    distanceR = lookR();
    delay(200);
    distanceL = lookL();
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
  
  distance = readPing();
  Uright_sensor = digitalRead(UR_S);
  Uleft_sensor = digitalRead(UL_S);
  if (Uleft_sensor != 0 && Uright_sensor != 0) {
    if (distance <= 8) {
      if (run != 2 ) {
        Stop();
        delay(100);
      }
      back();
    } else if (distance <= 25 && distance > 12) {
      if (run != 1 ) {
        Stop();
        delay(100);
      }
      forward();
    } else {
      Stop();
    }
  } else if (Uleft_sensor == 0 && Uright_sensor != 0) {
    if (run != 3 ) {
      Stop();
      delay(100);
    }
    right();
  } else if (Uleft_sensor != 0 && Uright_sensor == 0) {
    if (run != 4 ) {
      Stop();
      delay(100);
    }
    left();
  } else if (Uleft_sensor == 0 && Uright_sensor == 0) {
    Stop();
  }
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
  delay(100);
  myServo.write(gocTruoc);
  return distance;
}
int lookL() {
  myServo.write(gocTruoc + 50);
  delay(250);
  int distance = readPing();
  delay(100);
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
