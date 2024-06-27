#include <Arduino_FreeRTOS.h>
#include <NewPing.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <task.h>

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
#define MAX_DISTANCE 200  
#define gocTruoc 72

SoftwareSerial bluetooth(9, 8);
NewPing sonar(trig, echo, MAX_DISTANCE);
volatile Servo myServo;

TaskHandle_t xTaskHandleConnect;
TaskHandle_t xTaskHandleControl;

volatile char command;
volatile char temp = 'S';
volatile int Speed = 100;
volatile int mode = CONTROL_MODE;

volatile int run = 0;

volatile bool wireOn = true;
volatile bool bluetoothOn = true;
volatile bool isHandle = false;

String json = "";

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

  pinMode(DR_S, INPUT);
  pinMode(DL_S, INPUT);
  pinMode(UR_S, INPUT);
  pinMode(UL_S, INPUT);

  Wire.begin(100);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  xTaskCreate(vConnectTask, "Task1", 64, NULL, configMAX_PRIORITIES - 1, &xTaskHandleConnect);
  xTaskCreate(vHandleTask, "Task2", 400, NULL, configMAX_PRIORITIES - 1, &xTaskHandleControl);
  vTaskSuspend(xTaskHandleControl);

  delay(10);
  vTaskStartScheduler();
}

void loop() {
}

void vConnectTask(void* pvParameters) {
  for (;;) {
    if (bluetoothOn) {  
      if (bluetooth.available() > 0) {
        command = bluetooth.read();
        Serial.println(command);
        if (wireOn) {
          wireOn = false;
          Wire.end();
        }
        if(!isHandle) {
          isHandle = true;
          vTaskResume(xTaskHandleControl);
        }
      }
    } else {
      if (!wireOn) {
        wireOn = true;
        Wire.begin(100);
        Wire.onReceive(receiveEvent);
        Wire.onRequest(requestEvent);
      }
    }
  }
}

void vHandleTask(void* pvParameters) {
  for (;;) {
    if (command != temp) {
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
        myServo.write(gocTruoc);
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
      case 'D':
        bluetoothOn = false;
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
}
void AutoLine() {
  int Dleft_sensor;
  int Dright_sensor;
  Dleft_sensor = digitalRead(DL_S);
  Dright_sensor = digitalRead(DR_S);
  if (Dleft_sensor == 0 && Dright_sensor == 1) {
    if (run != 3) {
      Stop();
      back();
      vTaskDelay(100 / portTICK_PERIOD_MS);
      Stop();
    }
    right();
    json = "{\"Dl\":" + String(Dleft_sensor) + ",\"Dr\":" + String(Dright_sensor) + "}";
    Serial.println(json);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  } else if (Dleft_sensor == 1 && Dright_sensor == 0) {
    if (run != 4) {
      Stop();
      back();
      vTaskDelay(100 / portTICK_PERIOD_MS);
      Stop();
    }
    left();
    json = "{\"Dl\":" + String(Dleft_sensor) + ",\"Dr\":" + String(Dright_sensor) + "}";
    Serial.println(json);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  } else {
    if (run != 1) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    forward();
  }
}

void AutoObstacle() {
  int distance = 100;
  int distanceL = 100;
  int distanceR = 100;
  distance = readPing();
  if (distance <= 15) {
    Stop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    back();
    vTaskDelay(300 / portTICK_PERIOD_MS);
    Stop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    distanceR = lookR();
    vTaskDelay(200 / portTICK_PERIOD_MS);
    distanceL = lookL();
    json = "{\"d\":" + String(distance) + ",\"dR\":" + String(distanceR) + ",\"dL\":" + String(distanceL) + "}";
    Serial.println(json);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    if (distanceR >= distanceL) {
      Stop();
      right();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      Stop();
    } else {
      Stop();
      left();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      Stop();
    }
  } else {
    forward();
  }
}

void AutoFollow() {
  int distance = 100;
  int Uleft_sensor;
  int Uright_sensor;
  distance = readPing();
  Uright_sensor = digitalRead(UR_S);
  Uleft_sensor = digitalRead(UL_S);
  if (Uleft_sensor != 0 && Uright_sensor != 0) {
    if (distance <= 8) {
      if (run != 2) {
        Stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
      back();
    } else if (distance <= 25 && distance > 12) {
      if (run != 1) {
        Stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
      forward();
    } else {
      Stop();
    }
  } else if (Uleft_sensor == 0 && Uright_sensor != 0) {
    if (run != 3) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
    right();
  } else if (Uleft_sensor != 0 && Uright_sensor == 0) {
    if (run != 4) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
    left();
  } else if (Uleft_sensor == 0 && Uright_sensor == 0) {
    Stop();
  }
}

int readPing() {
  vTaskDelay(70 / portTICK_PERIOD_MS);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}

int lookR() {
  myServo.write(gocTruoc - 50);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  int distance = readPing();
  vTaskDelay(200 / portTICK_PERIOD_MS);
  myServo.write(gocTruoc);
  return distance;
}

int lookL() {
  myServo.write(gocTruoc + 50);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  int distance = readPing();
  vTaskDelay(200 / portTICK_PERIOD_MS);
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
  if (bluetoothOn) {
    bluetoothOn = false;
  }
  if(!isHandle) {
    isHandle = true;
    vTaskResume(xTaskHandleControl);
  }
  while (0 < Wire.available()) {
    command = Wire.read();
    Serial.println(command);
  }

  if(command == 'D') {
    wireOn = false;
    Wire.end();
    command = 'S';
    bluetoothOn = true;
  }
}

void requestEvent() {
  // if (json[0] == '{') {
    if (json.length() < 32) {
      for (int i = json.length() + 1; i <= 32; i++) {
        json += " ";
      }
    }
    Wire.write(json.c_str());
    json = "";
  // }
}
