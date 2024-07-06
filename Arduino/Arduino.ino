#include <Arduino_FreeRTOS.h>
#include <NewPing.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <task.h>
#include <queue.h>

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
#define MAX_DISTANCE 200
#define gocTruoc 72

SoftwareSerial bluetooth(9, 8);
NewPing sonar(trig, echo, MAX_DISTANCE);
Servo myServo;

TaskHandle_t xTaskHandleConnect;
TaskHandle_t xTaskHandleControl;
TaskHandle_t xTaskHandleData;
TaskHandle_t xTaskAutoLine = NULL;
TaskHandle_t xTaskAutoFollow = NULL;
TaskHandle_t xTaskAutoObstacle = NULL;

QueueHandle_t queueCommand;
QueueHandle_t queueJson;

byte Speed = 100;

byte run = 0;

bool wifiOn = true;
bool bluetoothOn = true;
bool isHandle = false;
bool isAuto = false;

void forward();
void back();
void left();
void right();

void setup() {
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(DR_S, INPUT);
  pinMode(DL_S, INPUT);
  pinMode(UR_S, INPUT);
  pinMode(UL_S, INPUT);
  myServo.attach(servoPin);
  bluetooth.begin(9600);
  Wire.begin(100);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  queueCommand = xQueueCreate(3, sizeof(char));
  queueJson = xQueueCreate(3, sizeof(String));

  xTaskCreate(vConnectTask, "Task1", 60, NULL, configMAX_PRIORITIES - 1, &xTaskHandleConnect);
  xTaskCreate(vHandleData, "Task2", 60, NULL, configMAX_PRIORITIES - 1, &xTaskHandleData);
  vTaskSuspend(xTaskHandleData);
  xTaskCreate(vHandleControl, "Task3", 64, NULL, configMAX_PRIORITIES - 1, &xTaskHandleControl);
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
        char command = bluetooth.read();
        xQueueSendToBack(queueCommand, &command, portMAX_DELAY);
        Serial.println(command);
        if (wifiOn) {
          wifiOn = false;
          Wire.end();
        }
        if (!isHandle) {
          isHandle = true;
          vTaskResume(xTaskHandleData);
        }
      }
    } else {
      if (wifiOn) {
        wifiOn = true;
        Wire.begin(100);
        Wire.onReceive(receiveEvent);
        Wire.onRequest(requestEvent);
      }
    }
  }
}

void vHandleData(void* pvParameters) {
  char command, temp;
  for (;;) {
    if (xQueueReceive(queueCommand, &command, portMAX_DELAY) == pdPASS) {
      if (command != temp) {
        Stop();
        if (isAuto) {
          isAuto = false;
          // vTaskDelete(xTaskAutoLine);
          // vTaskDelete(xTaskAutoObstacle);
          // vTaskDelete(xTaskAutoFollow);
          vTaskDelete(xTaskHandleControl);
        }
      }
    }

    switch (command) {
      case 'Y':
        myServo.detach();
        Speed = 100;
        temp = 'Y';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoLine, "Task4", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoLine);
        }
        break;
      case 'Z':
        myServo.attach(servoPin);
        Speed = 100;
        temp = 'Z';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoObstacle, "Task5", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoObstacle);
        }
        myServo.write(gocTruoc);
        break;
      case 'T':
        myServo.attach(servoPin);
        Speed = 100;
        temp = 'T';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoFollow, "Task6", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoFollow);
        }
        myServo.write(gocTruoc);
        break;
      case 'D':
        bluetoothOn = false;
        vTaskSuspend(NULL);
        break;
      default:
        isAuto = false;
        myServo.detach();
        xQueueSendToFront(queueCommand, &command, portMAX_DELAY);
        vTaskResume(xTaskHandleControl);
        vTaskSuspend(NULL);
    }
  }
}

void vHandleControl(void* pvParameters) {
  char command, temp;
  for (;;) {
    if (xQueueReceive(queueCommand, &command, portMAX_DELAY) == pdPASS) {
      if (command != temp) {
        Stop();
      }
    }

    switch (command) {
      case 'F':
        Speed = 150;
        forward();
        temp = 'F';
        break;
      case 'B':
        Speed = 150;
        back();
        temp = 'B';
        break;
      case 'L':
        Speed = 150;
        left();
        temp = 'L';
        break;
      case 'R':
        Speed = 150;
        right();
        temp = 'R';
        break;
      case 'S':
        Stop();
        temp = 'S';
        break;
      case 'D':
        bluetoothOn = false;
        vTaskSuspend(NULL);
        break;
      default:
        isAuto = true;
        xQueueSendToFront(queueCommand, &command, portMAX_DELAY);
        vTaskResume(xTaskHandleData);
        vTaskSuspend(NULL);
    }
  }
}


void vAutoLine(void* pvParameters) {
  int Dleft_sensor;
  int Dright_sensor;
  for (;;) {
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
      if (wifiOn) {
        String json = "{\"Dl\":" + String(Dleft_sensor) + ",\"Dr\":" + String(Dright_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    } else if (Dleft_sensor == 1 && Dright_sensor == 0) {
      if (run != 4) {
        Stop();
        back();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Stop();
      }
      left();
      if (wifiOn) {
        String json = "{\"Dl\":" + String(Dleft_sensor) + ",\"Dr\":" + String(Dright_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    } else {
      if (run != 1) {
        Stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      forward();
    }
  }
}

void vAutoObstacle() {
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
    if (wifiOn) {
      String json = "{\"d\":" + String(distance) + ",\"dR\":" + String(distanceR) + ",\"dL\":" + String(distanceL) + "}";
      xQueueSendToBack(queueJson, &json, portMAX_DELAY);
    }
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

void vAutoFollow() {
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
      if (wifiOn) {
        String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      back();
    } else if (distance <= 25 && distance > 12) {
      if (run != 1) {
        Stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      if (wifiOn) {
        String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      forward();
    } else {
      Stop();
    }
  } else if (Uleft_sensor == 0 && Uright_sensor != 0) {
    if (run != 3) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (wifiOn) {
      String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
      xQueueSendToBack(queueJson, &json, portMAX_DELAY);
    }
    right();
  } else if (Uleft_sensor != 0 && Uright_sensor == 0) {
    if (run != 4) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (wifiOn) {
      String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
      xQueueSendToBack(queueJson, &json, portMAX_DELAY);
    }
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
  char command;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (bluetoothOn) {
    bluetoothOn = false;
  }
  if (!isHandle) {
    isHandle = true;
    vTaskResume(xTaskHandleControl);
  }
  while (0 < Wire.available()) {
    command = Wire.read();
    Serial.println(command);
    xQueueSendToBackFromISR(queueCommand, &command, &xHigherPriorityTaskWoken);
  }

  if (command == 'D') {
    wifiOn = false;
    Wire.end();
    command = 'S';
    bluetoothOn = true;
  }

  if (xHigherPriorityTaskWoken) {
    taskYIELD();
  }
}

void requestEvent() {
  String json;
  BaseType_t xTaskWokenByReceive = pdFALSE;
  if (xQueueReceiveFromISR(queueJson, &json, &xTaskWokenByReceive)) {
    if (json.length() < 32) {
      for (int i = json.length() + 1; i <= 32; i++) {
        json += " ";
      }
    }
    Wire.write(json.c_str());
  }
  if (xTaskWokenByReceive != pdFALSE) {
    taskYIELD();
  }
}
