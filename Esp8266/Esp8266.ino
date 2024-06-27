#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>

const char *ssid = "student";
const char *password = ""; 
std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
String data;
char temp = 'A';
SocketIOclient socketIO;

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            socketIO.send(sIOtype_CONNECT, "/car-active");
            break;
        case sIOtype_EVENT:
            data = String((char *)payload);
            if(data.indexOf("parameters") == -1) {
              temp = data[31];
              Wire.beginTransmission(100);
              Wire.write(temp); 
              Wire.endTransmission();
            }
            break;
        case sIOtype_ACK:
            Serial.printf("[IOc] get ack: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_ERROR:
            Serial.printf("[IOc] get error: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_EVENT:
            Serial.printf("[IOc] get binary: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_ACK:
            Serial.printf("[IOc] get binary ack: %u\n", length);
            hexdump(payload, length);
            break;
    }
}

void setup() {
  Serial.begin(9600); 
  Wire.begin(D1, D2);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  client->setInsecure();
  socketIO.beginSSL("bugnef-be-xedieukhien.onrender.com", 443, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop();
  if(temp == 'Y' || temp == 'Z' || temp == 'T') {
    getAndPutData();
  }
}

void getAndPutData() {
  char c;
  String json = "";
  Wire.requestFrom(100, 32);
  delay(100);
  while(Wire.available() > 0){
    c = Wire.read();
    json += c;
  }
  if(json[0] != ' ' && json[1] != ' ') {
    String payload = "/car-active,[\"update-parameters\"," + json + "]";
    Serial.println(payload);
    socketIO.sendEVENT(payload);
  }
 }