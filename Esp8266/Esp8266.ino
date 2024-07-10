#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ctype.h>

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
  String data = "";
  String json = "";
  Wire.requestFrom(100, 11);
  while(Wire.available() > 0){
    c = Wire.read();
    data += c;
  }
  Serial.println(data);
  if(isDigit(data[0])) {
    Serial.println("1");
    size_t index;
    for(index = data.length() - 1; index >= 0; index--) {
      if(isDigit(data[index])) {
        break;
      }
    }
    data = data.substring(0, index + 1);
    index = data.indexOf(' ');
    if(temp == 'Y') {
      String Dleft_sensor = data.substring(0, index);
      String Dright_sensor = data.substring(index + 1);
      json = "{\"Dl\":" + Dleft_sensor + ",\"Dr\":" + Dright_sensor + "}";
    }else if(temp == 'Z') {
      String distance = data.substring(0, index);
      size_t lastIndex = data.lastIndexOf(' ');
      String distanceR = data.substring(index + 1, lastIndex);
      String distanceL = data.substring(lastIndex + 1);
      json = "{\"d\":" + distance + ",\"dR\":" + distanceR + ",\"dL\":" + distanceL + "}";
    }else {
      String distance = data.substring(0, index);
      size_t lastIndex = data.lastIndexOf(' ');
      String Uright_sensor = data.substring(index + 1, lastIndex);
      String Uleft_sensor = data.substring(lastIndex + 1);
      json = "{\"d\":" + distance + ",\"Ur\":" + Uright_sensor + ",\"Ul\":" + Uleft_sensor + "}";
    }

    String payload = "/car-active,[\"update-parameters\"," + json + "]";
    Serial.println(payload);
    socketIO.sendEVENT(payload);
  }
 }