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
#define ECHO A2
#define TRIG A3
#define Servo_PIN 7
#define MAX_DISTANCE 200
#define FRONT_ANGLE 72
#define CONTROL_SPEED 150
#define AUTO_SPEED 100

SoftwareSerial bluetooth(9, 8);
NewPing sonar(TRIG, ECHO, MAX_DISTANCE);
Servo myServo;

TaskHandle_t xTaskHandleConnect;
TaskHandle_t xTaskHandleControl;
TaskHandle_t xTaskHandleData;
TaskHandle_t xTaskAutoLine;
TaskHandle_t xTaskAutoFollow;
TaskHandle_t xTaskAutoObstacle;

QueueHandle_t queueCommand;
QueueHandle_t queueJson;

byte run = 0;

bool wifiOn = true;
bool bluetoothOn = true;
bool isHandle = false;
bool isAuto = false;

void setup() {
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(DR_S, INPUT);
  pinMode(DL_S, INPUT);
  pinMode(UR_S, INPUT);
  pinMode(UL_S, INPUT);
  myServo.attach(Servo_PIN);
  bluetooth.begin(9600);
  Wire.begin(100);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  queueCommand = xQueueCreate(3, sizeof(char));
  queueJson = xQueueCreate(3, sizeof(String));

  xTaskCreate(vConnectTask, "Task1", 60, NULL, configMAX_PRIORITIES - 1, &xTaskHandleConnect);
  xTaskCreate(vHandleData, "Task2", 64, NULL, configMAX_PRIORITIES - 1, &xTaskHandleData);
  vTaskSuspend(xTaskHandleData);

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
  char command = NULL;
  char temp = NULL;
  for (;;) {
    if (xQueueReceive(queueCommand, &command, portMAX_DELAY) == pdPASS) {
      if (command != temp && temp != NULL) {
        Stop();
        if (isAuto) {
          isAuto = false;
          switch (temp) {
            case 'T':
              vTaskDelete(xTaskAutoFollow);
              break;
            case 'Y':
              vTaskDelete(xTaskAutoLine);
              break;
            case 'Z':
              vTaskDelete(xTaskAutoObstacle);
              break;
            default:
              vTaskDelete(xTaskHandleControl);
          }
        }
      }
    }

    switch (command) {
      case 'D':
        bluetoothOn = false;
        vTaskSuspend(NULL);
        break;
      case 'T':
        myServo.attach(Servo_PIN);
        temp = 'T';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoFollow, "Task6", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoFollow);
        }
        myServo.write(FRONT_ANGLE);
        break;
      case 'Y':
        myServo.detach();
        temp = 'Y';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoLine, "Task4", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoLine);
        }
        break;
      case 'Z':
        myServo.attach(Servo_PIN);
        temp = 'Z';
        if (!isAuto) {
          isAuto = true;
          xTaskCreate(vAutoObstacle, "Task5", 198, NULL, configMAX_PRIORITIES - 1, &xTaskAutoObstacle);
        }
        myServo.write(FRONT_ANGLE);
        break;
      default:
        isAuto = false;
        temp = 'F';
        myServo.detach();
        xQueueSendToFront(queueCommand, &command, portMAX_DELAY);
        xTaskCreate(vHandleControl, "Task3", 64, NULL, configMAX_PRIORITIES - 1, &xTaskHandleControl);
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
      case 'B':
        back(CONTROL_SPEED);
        temp = 'B';
        break;
      case 'D':
        bluetoothOn = false;
        vTaskSuspend(NULL);
        break;
      case 'F':
        forward(CONTROL_SPEED);
        temp = 'F';
        break;
      case 'L':
        left(CONTROL_SPEED);
        temp = 'L';
        break;
      case 'R':
        right(CONTROL_SPEED);
        temp = 'R';
        break;
      case 'S':
        Stop();
        temp = 'S';
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
  byte Dleft_sensor;
  byte Dright_sensor;
  for (;;) {
    Dleft_sensor = digitalRead(DL_S);
    Dright_sensor = digitalRead(DR_S);
    if (Dleft_sensor == 0 && Dright_sensor == 1) {
      if (run != 3) {
        Stop();
        back(AUTO_SPEED);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Stop();
      }
      right(AUTO_SPEED);
      if (wifiOn) {
        String json = "{\"Dl\":" + String(Dleft_sensor) + ",\"Dr\":" + String(Dright_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    } else if (Dleft_sensor == 1 && Dright_sensor == 0) {
      if (run != 4) {
        Stop();
        back(AUTO_SPEED);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Stop();
      }
      left(AUTO_SPEED);
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
      forward(AUTO_SPEED);
    }
  }
}

void vAutoObstacle() {
  byte distance = 100;
  byte distanceL = 100;
  byte distanceR = 100;
  for (;;) {
    distance = readPing();
    if (distance <= 15) {
      Stop();
      vTaskDelay(100 / portTICK_PERIOD_MS);
      back(AUTO_SPEED);
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
        right(AUTO_SPEED);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Stop();
      } else {
        Stop();
        left(AUTO_SPEED);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Stop();
      }
    } else {
      forward(AUTO_SPEED);
    }
  }
}

void vAutoFollow() {
  byte distance = 100;
  byte Uleft_sensor;
  byte Uright_sensor;
  for (;;) {
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
        back(AUTO_SPEED);
      } else if (distance <= 25 && distance > 12) {
        if (run != 1) {
          Stop();
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        if (wifiOn) {
          String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
          xQueueSendToBack(queueJson, &json, portMAX_DELAY);
        }
        forward(AUTO_SPEED);
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
      right(AUTO_SPEED);
    } else if (Uleft_sensor != 0 && Uright_sensor == 0) {
      if (run != 4) {
        Stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      if (wifiOn) {
        String json = "{\"d\":" + String(distance) + ",\"Ur\":" + String(Uright_sensor) + ",\"Ul\":" + String(Uleft_sensor) + "}";
        xQueueSendToBack(queueJson, &json, portMAX_DELAY);
      }
      left(AUTO_SPEED);
    } else if (Uleft_sensor == 0 && Uright_sensor == 0) {
      Stop();
    }
  }
}

byte readPing() {
  vTaskDelay(70 / portTICK_PERIOD_MS);
  byte cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}

byte lookR() {
  myServo.write(FRONT_ANGLE - 50);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  byte distance = readPing();
  vTaskDelay(200 / portTICK_PERIOD_MS);
  myServo.write(FRONT_ANGLE);
  return distance;
}

byte lookL() {
  myServo.write(FRONT_ANGLE + 50);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  byte distance = readPing();
  vTaskDelay(200 / portTICK_PERIOD_MS);
  myServo.write(FRONT_ANGLE);
  return distance;
}

void left(byte speed) {
  run = 4;
  analogWrite(IN1, speed);
  analogWrite(IN3, speed);
}
void right(byte speed) {
  run = 3;
  analogWrite(IN2, speed);
  analogWrite(IN4, speed);
}
void back(byte speed) {
  run = 2;
  analogWrite(IN3, speed);
  analogWrite(IN2, speed);
}

void forward(byte speed) {
  run = 1;
  analogWrite(IN4, speed);
  analogWrite(IN1, speed);
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
