#include <Arduino.h>
#include <WiFi.h>
#include "settings.h"


#define pos1Addr 0 //size is sizeof("0,000000000")
#define pos2Addr pos1Addr + sizeof("00000000000") // size is sizeof("0,000000000")
#define DirtyFlagAddress pos2Addr + sizeof("00000000000") + sizeof(int)//size is one int
#define DirtyMessageAddress DirtyFlagAddress + sizeof(int) // For now we dont have a size for this

#include <HTTPClient.h>

#define ASYNC

#ifndef ASYNC
#include <WebServer.h>
#else
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif

#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "newBase64.h"

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
unsigned long interval = 10000;

unsigned long heartBeat_previousMillis = 0;
unsigned long heartBeatInterval = 60*60000;

unsigned long previousRSSIDMillis = 0;
unsigned long RSSIDinterval = 10000;

bool eepromStarted = false;

#ifndef ASYNC
WebServer server(GateSettings::ServerPort);
#else
AsyncWebServer server(GateSettings::ServerPort);

#endif
const int sleepTime = 500;


void logtoserver(String msg){
  if(GateSettings::LoggingEnabled == false)
    return;
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(GateSettings::LogURL.c_str());

      // Specify content-type header
      // http.addHeader("Content-Type", "text/plain");
      // Data to send with HTTP POST
      // String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14";           
      // Send HTTP POST request
      // int httpResponseCode = http.POST(httpRequestData);
      
      // If you need an HTTP request with a content type: application/json, use the following:

      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST("{\"logmsg\":\"[ESP32-GWS] " + msg + "\"}");
      // Serial.println("{\"logmsg\":\"[ESP32-GWS] " + msg + "\"}");
      // If you need an HTTP request with a content type: text/plain
      //http.addHeader("Content-Type", "text/plain");
      //int httpResponseCode = http.POST("Hello, World!");
        
      // Free resources
      http.end();
    }
}

String pos1 = "";
String pos2 = "";
bool updated = false;

void shutdownWifi(){
  WiFi.disconnect();
  delay(5 *1000);
}

void initWiFi(){
  bool staticip = true;
  if (!WiFi.config(GateSettings::local_IP, GateSettings::gateway, GateSettings::subnet)){
    staticip = false;
   //Serial.println("STA Failed to configure");
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(GateSettings::WiFiSSID.c_str(), GateSettings::WiFiPassword.c_str());
  //Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
  }

  //WiFi.setHostname(hostname); //define hostname
  logtoserver("connected to wifi");
  if(!staticip)
    logtoserver("Failed to assign static ip.");

}

void switch_pin(int pinNumber){
  // logtoserver("starting to switch pin " + String(pinNumber));
  digitalWrite(pinNumber, LOW);
  delay(sleepTime);
  digitalWrite(pinNumber, HIGH);
  // logtoserver("done switching pin " + String(pinNumber));
}

#ifndef ASYNC
void open_gate(){
  logtoserver("starting to open gate");
  server.send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_open);
  logtoserver("done opening gate");
  //Serial.println("Opening gate");
}

void close_gate(){
  logtoserver("starting to close gate");
  server.send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_close);
  logtoserver("done to close gate");
  //Serial.println("Closing gate");
}

void stop_gate(){
  logtoserver("starting to stop gate");
  server.send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_stop);
  logtoserver("done to stop gate");
  //Serial.println("Stopping gate");
}

void pedestrian_gate(){
  logtoserver("starting to pedestrian gate");
  server.send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_ped);
  logtoserver("done to pedestrian gate");
  //Serial.println("Pedestrian mode");
}

void handle_default(){
  logtoserver("default data begin");
  server.send(200, "text/plain", server.arg(0));
  logtoserver("default data end");
  //Serial.println("WS");
}

void handle_serve_position(){
  logtoserver("begin serving position");
  int len1 = NBase64.encodedLength(pos1.length());
  char pos1E[len1];
  NBase64.encode(pos1E, (char*)pos1.c_str(), pos1.length());
  String pos1Encoded = pos1E;

  int len2 = NBase64.encodedLength(pos2.length());
  char pos2E[len2];
  NBase64.encode(pos2E, (char*)pos2.c_str(), pos2.length());
  String pos2Encoded = pos2E;
  server.send(200, "text", pos1Encoded+"|"+pos2Encoded);
  logtoserver("done serving position");
}

void handle_notFound(){
  logtoserver("not found begin");
  server.send(404, "text/plain", server.uri());
  if (server.uri().startsWith("/SavePOS")){
    logtoserver("not found saving pos begin");
    String pos = server.uri();
    pos.replace("/SavePOS", "");
    int decodedLength = NBase64.decodedLength((char*)pos.c_str(), pos.length());
    char decoded[decodedLength];
    NBase64.decode(decoded, (char*)pos.c_str(), pos.length());
    String decodedPos = decoded;
    int separatorIndex = decodedPos.indexOf('|');
    pos1 = decodedPos.substring(0, separatorIndex);
    pos2 = decodedPos.substring(separatorIndex+1, pos.length());
    pos1 = pos1.substring(0, 11);
    pos2 = pos2.substring(0, 11);
    EEPROM.writeString(0, pos1);
    EEPROM.writeString(256, pos2);
    EEPROM.commit();
    logtoserver("not found saving pos end");
  }
  logtoserver("not found end");
}
#endif

#pragma region asyncWS
#ifdef ASYNC

void handle_notFound(AsyncWebServerRequest *request){
  // logtoserver("not found begin");
  request->send(404, "text/plain", request->url());
  if (request->url().startsWith("/SavePOS")){
    // logtoserver("not found saving pos begin");
    String pos = request->url();
    pos.replace("/SavePOS", "");
    int decodedLength = NBase64.decodedLength((char*)pos.c_str(), pos.length());
    char decoded[decodedLength];
    NBase64.decode(decoded, (char*)pos.c_str(), pos.length());
    String decodedPos = decoded;
    int separatorIndex = decodedPos.indexOf('|');
    pos1 = decodedPos.substring(0, separatorIndex);
    pos2 = decodedPos.substring(separatorIndex+1, pos.length());
    pos1 = pos1.substring(0, 11);
    pos2 = pos2.substring(0, 11);
    EEPROM.writeString(pos1Addr, pos1);
    EEPROM.writeString(pos2Addr, pos2);
    EEPROM.commit();
    // logtoserver("not found saving pos end");
  }
  // logtoserver("not found end");
}

void open_gate(AsyncWebServerRequest *request){
  logtoserver("starting to open gate");
  request->send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_open);
  logtoserver("done opening gate");
  //Serial.println("Opening gate");
}

void close_gate(AsyncWebServerRequest *request){
  logtoserver("starting to close gate");
  request->send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_close);
  logtoserver("done to close gate");
  //Serial.println("Closing gate");
}

void stop_gate(AsyncWebServerRequest *request){
  logtoserver("starting to stop gate");
  request->send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_stop);
  logtoserver("done to stop gate");
  //Serial.println("Stopping gate");
}

void pedestrian_gate(AsyncWebServerRequest *request){
  logtoserver("starting to pedestrian gate");
  request->send(200, "text/plain", "Status OK!");
  switch_pin(GateSettings::gpio_gate_ped);
  logtoserver("done to pedestrian gate");
  //Serial.println("Pedestrian gate mode");
}

void serve_position(AsyncWebServerRequest *request){
  logtoserver("begin serving position");

  int len1 = NBase64.encodedLength(pos1.length());
  char pos1E[len1];
  NBase64.encode(pos1E, (char*)pos1.c_str(), pos1.length());
  String pos1Encoded = pos1E;

  int len2 = NBase64.encodedLength(pos2.length());
  char pos2E[len2];
  NBase64.encode(pos2E, (char*)pos2.c_str(), pos2.length());
  String pos2Encoded = pos2E;
  request->send(200, "text", pos1Encoded+"|"+pos2Encoded);
  logtoserver("done serving position");
}

#endif
#pragma endregion

void WriteDirtyMessage(String message){
  if (!eepromStarted){
    eepromStarted = EEPROM.begin(1024); //Make sure EEPROM is initialized
  }
  EEPROM.writeInt(DirtyFlagAddress, 1); //Set the dirty flag
  EEPROM.writeString(DirtyMessageAddress, message); // Write the message
  EEPROM.commit();
}

void WiFi_DisconnectedEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  WriteDirtyMessage("Disconnected from WiFi. Was this intentional");
  ESP.restart();
}

void WiFi_LostIPEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  WriteDirtyMessage("We lost the IP address. Was this caused due to disconnect");
  shutdownWifi();
  ESP.restart();
}

void setup() {
  eepromStarted = EEPROM.begin(1024);
  if (EEPROM.readString(pos1Addr) == ""){
    EEPROM.writeString(pos1Addr, "0,00000000");
  }
  if (EEPROM.readString(pos2Addr) == ""){
    EEPROM.writeString(pos2Addr, "0,00000000");
  }
  pos1 = EEPROM.readString(pos1Addr);
  pos2 = EEPROM.readString(pos2Addr);

  pinMode(GateSettings::gpio_gate_open, OUTPUT);
  pinMode(GateSettings::gpio_gate_close, OUTPUT);
  pinMode(GateSettings::gpio_gate_stop, OUTPUT);
  pinMode(GateSettings::gpio_gate_ped, OUTPUT);
  digitalWrite(GateSettings::gpio_gate_open, HIGH);
  digitalWrite(GateSettings::gpio_gate_close, HIGH);
  digitalWrite(GateSettings::gpio_gate_stop, HIGH);
  digitalWrite(GateSettings::gpio_gate_ped, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initWiFi();

  #ifndef ASYNC
  server.on("/", handle_default);
  server.on(GateSettings::OpenGate.c_str(), open_gate);
  server.on(GateSettings::CloseGate.c_str(), close_gate);
  server.on(GateSettings::StopGate.c_str(), stop_gate);
  server.on(GateSettings::PedestrianGate.c_str(), pedestrian_gate);
  server.on(GateSettings::GetPositionGate.c_str(), handle_serve_position);
  server.onNotFound(handle_notFound);
  
  #else
  server.onNotFound(handle_notFound);
  server.on(GateSettings::OpenGate.c_str(), HTTP_GET, open_gate);
  server.on(GateSettings::CloseGate.c_str(), HTTP_GET, close_gate);
  server.on(GateSettings::StopGate.c_str(), HTTP_GET, stop_gate);
  server.on(GateSettings::PedestrianGate.c_str(), HTTP_GET, pedestrian_gate);
  server.on(GateSettings::GetPosition.c_str(), HTTP_GET, serve_position);
  #endif

  ArduinoOTA.setPassword(GateSettings::OTAPassword.c_str());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      ESP.restart();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
    })
    .onError([](ota_error_t error) {
      //Serial.printf("Error[%u]: ", error);
      //if (error == OTA_AUTH_ERROR) //Serial.println("Auth Failed");
      //else if (error == OTA_BEGIN_ERROR) //Serial.println("Begin Failed");
      //else if (error == OTA_CONNECT_ERROR) //Serial.println("Connect Failed");
      //else if (error == OTA_RECEIVE_ERROR) //Serial.println("Receive Failed");
      //else if (error == OTA_END_ERROR) //Serial.println("End Failed");
    });
  // xTaskCreatePinnedToCore(rebootExecutor, "rebootTask", 10000, NULL, 0, &rebootChecker, 0);

  WiFi.onEvent(WiFi_LostIPEvent, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.onEvent(WiFi_DisconnectedEvent, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  if(EEPROM.readInt(DirtyFlagAddress) == 1){
    String dirtyMsg = EEPROM.readString(DirtyMessageAddress);
    logtoserver("Dirty flag was triggered");
    logtoserver(dirtyMsg);
    EEPROM.writeInt(DirtyFlagAddress, 0);
    EEPROM.commit();
  }
  ArduinoOTA.begin();
  server.begin();
}

void loop() {
  #ifndef ASYNC
  server.handleClient();
  #else
  
  #endif
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();
  
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    //Serial.print(millis());
    //Serial.println("Reconnecting to WiFi...");
    ESP.restart();
    previousMillis = currentMillis;
  }
  if((WiFi.status() == WL_CONNECTED) && (currentMillis - heartBeat_previousMillis >= heartBeatInterval)){
    logtoserver(WiFi.localIP().toString());
    heartBeat_previousMillis = currentMillis;
  }

  if(WiFi.status() != WL_CONNECTED)
    digitalWrite(LED_BUILTIN, LOW);
  else{
    digitalWrite(LED_BUILTIN, HIGH);
  }
}