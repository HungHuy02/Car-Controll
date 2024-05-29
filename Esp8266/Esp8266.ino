#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <ArduinoJson.h>

const char *ssid = "Student";
const char *password = ""; 
const char *apiEndpoint = "https://bugnef-be-xedieukhien.onrender.com/cars";
const char *apiUpdate = "https://bugnef-be-xedieukhien.onrender.com/parameters/update";
std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
HTTPClient http;
String data;
char c;

void setup() {
  Serial.begin(115200);
  Wire.begin(D1, D2);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  client->setInsecure();
  http.begin(*client ,apiEndpoint);
}

void loop() {
  getAndSendData();
 if(data.equals("Y") || data.equals("Z") || data.equals("T")) {
  getAndPutData();
 }
}

void getAndSendData() {
  int httpResponseCode = http.GET();
  String payload = "{}"; 
   DynamicJsonDocument jsonDoc(256);
   Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
     deserializeJson(jsonDoc, http.getString());
      data = String(jsonDoc["data"]);
      Wire.beginTransmission(100); 
      Wire.write(data.c_str()); 
      Wire.endTransmission();
  }
}

void getAndPutData() {
  char c;
  String json = "";
    Wire.requestFrom(100, 32);
     while(Wire.available() > 0){
      c = Wire.read();
      json += c;
   }
      http.end();
    http.begin(*client ,apiUpdate);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.PUT(json);
    String response = http.getString();
    http.end();
    http.begin(*client ,apiEndpoint);
 }