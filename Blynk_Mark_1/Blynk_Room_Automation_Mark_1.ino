#define BLYNK_DEBUG
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>
#include "DHT.h"

#define DHTPIN D1
#define DHTTYPE DHT11

SimpleTimer timer;
ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

//Network Info
const char* ssid = "InsertYourSSID";
const char* password = "InsertYourPassword";

//Blynk
char auth[] = "InsertYourBlynkAuthenticationCode";

//  Hue constants 
const char hueHubIP[] = "InsertYourLocalHueHubIPAddress";  // Hue hub IP
const char hueUsername[] = "InsertYourHueUsernameHere";  // Hue username
const int hueHubPort = 80;

//  Hue variables
String command;

static boolean nextRoomLightState = 0;
static boolean currentRoomLightState = 0;

static boolean nextLEDState = 0;
static boolean currentLEDState = 0;

static boolean nextLampState = 0;
static boolean currentLampState = 0;

static int currDimValue = 255;
static int prevDimValue = 255;

static boolean blindsButton = false;
static boolean blindsOpen = false;
const long blindsInterval = 8000;
static unsigned long prevBlindsMillis = 0;
static boolean blindsWebRequest = false;
static boolean prevBlindsButton = false;

String webPage = "";

BLYNK_WRITE(V0){
  nextRoomLightState = param.asInt(); // assigning incoming value from pin V0 to a variable
}

BLYNK_WRITE(V1){
  nextLEDState = param.asInt(); // assigning incoming value from pin V1 to a variable
}

BLYNK_WRITE(V3){
  nextLampState = param.asInt(); // assigning incoming value from pin V3 to a variable
}

BLYNK_WRITE(V4){
  currDimValue = param.asInt(); // assigning incoming value from pin V4 to a variable
}

BLYNK_WRITE(V6){
  blindsButton = param.asInt(); // assigning incoming value from pin V6 to a variable
}

void setup(void){  
  //Blynk
  Blynk.begin(auth, ssid, password);
  pinMode(V0, INPUT); //room lights
  pinMode(V1, INPUT); //LEDs
  pinMode(V3, INPUT); //lamp
  pinMode(V4, INPUT); //dim
  pinMode(D2, OUTPUT); //Physical LED to Arduino UNO  
  digitalWrite(D2, LOW);
  dht.begin();
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  timer.setInterval(500L, updateAllSystems);
  timer.setInterval(10000L, reconnectBlynk);
  timer.setInterval(60000L, updateTempAndHumidity);
  
  webPage += "<a href=\"LEDon\"><button>LED On</button></a><br>";
  webPage += "<a href=\"LEDoff\"><button>LED Off</button></a><br>";
  webPage += "<a href=\"lamp_on\"><button>Lamp On</button></a><br>";
  webPage += "<a href=\"lamp_off\"><button>Lamp Off</button></a><br>";
  webPage += "<a href=\"room_light_on\"><button>Room Light On</button></a><br>";
  webPage += "<a href=\"room_light_off\"><button>Room Light Off</button></a><br>";
  
  //Root Site
  server.on("/", [](){
    server.send(200, "text/html", webPage);
  });

  server.on("/LEDon", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");        
    nextLEDState = 1;
    LEDon();
  });
 
  server.on("/LEDoff", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");        
    nextLEDState = 0;
    LEDoff();
  });

  server.on("/lamp_on", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");        
    nextLampState = 1;
    lamp_on();
  });

  server.on("/lamp_off", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");        
    nextLampState = 0;
    lamp_off();
    
  });

  server.on("/room_light_on", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
    nextRoomLightState = 1;
    roomLightOn();
  });
 
  server.on("/room_light_off", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");        
    nextRoomLightState = 0;
    roomLightOff();
  });


  server.on("/open_blinds", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
    if(!blindsOpen){
      blindsWebRequest = true;
      toggleBlinds();  
    }
  });

  server.on("/close_blinds", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
    if(blindsOpen){
      blindsWebRequest = true;
      toggleBlinds();  
    }
  });

  server.on("/max_brightness", [](){
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
    currDimValue = 255;
    Blynk.virtualWrite(V4, 255);
  });
  
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  if(Blynk.connected()){
    Blynk.run();
  }
  timer.run();
  server.handleClient();
}

void reconnectBlynk() {
  if (!Blynk.connected()) {
    if(Blynk.connect()) {
      BLYNK_LOG("Reconnected");
    } else {
      BLYNK_LOG("Not reconnected");
    }
  }
}
  
void updateAllSystems(){
    updateRoomLights();
    updateLED();
    updateLamp();
    updateDim();
    toggleBlinds();
}

void toggleBlinds(){
  unsigned long currBlindsMillis = millis();
  if((blindsButton != prevBlindsButton || blindsWebRequest) && (currBlindsMillis - prevBlindsMillis >= blindsInterval)){
    prevBlindsMillis = currBlindsMillis;
    if(blindsOpen){//if currently open, then close
      HTTP_GET_REQ("192.168.1.15", "/close"); 
      Blynk.virtualWrite(V6, LOW);
    }else{//otherwise, must be closed, so open
      HTTP_GET_REQ("192.168.1.15", "/open");  
      Blynk.virtualWrite(V6, HIGH);
    }
    blindsOpen = !blindsOpen;
    blindsWebRequest = false;
    prevBlindsButton = blindsButton;
  }
}

void updateTempAndHumidity(){
  float t = dht.readTemperature(true); //true = Fahrenheit, false = Celsius
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)){return;}
  Blynk.virtualWrite(V10, t);
  Blynk.virtualWrite(V11, h);
}

void updateDim(){
  if(currDimValue != prevDimValue){
    command = "{\"bri\":" + String(currDimValue) + "}";
    setHue(1,command);
    setHue(2,command);
    Serial.println(currDimValue);
    prevDimValue = currDimValue;
  }
}

void updateLamp(){
  //if light should be on, turn on light
  if(nextLampState == 1 && currentLampState == 0){
    lamp_on();
  }//if light should be off, turn off light 
  else if(nextLampState == 0 && currentLampState == 1){
    lamp_off();
  }
}

//Virtual Pin V3 == lamp
void lamp_on(){
    Serial.println("lamp_on");
    currentLampState = 1;
    HTTP_GET_REQ("192.168.1.5", "/lightOn");
    Blynk.virtualWrite(3, 1);
    Blynk.syncVirtual(3);
}

void lamp_off(){
    Serial.println("lamp_off");
    currentLampState = 0; 
    HTTP_GET_REQ("192.168.1.5", "/lightOff");
    Blynk.virtualWrite(3, 0);
    Blynk.syncVirtual(3);
}

void updateLED(){
  //if light should be on, turn on light
  if(nextLEDState == 1 && currentLEDState == 0){
    LEDon();
  }//if light should be off, turn off light 
  else if(nextLEDState == 0 && currentLEDState == 1){
    LEDoff();
  }
}

//Virtual Pin V1 == LED
void LEDon(){
    Serial.println("LEDon");
    digitalWrite(D2, HIGH);
    currentLEDState = 1;
    Blynk.virtualWrite(1, 1);
    Blynk.syncVirtual(1);
}

void LEDoff(){
    Serial.println("LEDoff");
    digitalWrite(D2, LOW);
    currentLEDState = 0; 
    Blynk.virtualWrite(1, 0);
    Blynk.syncVirtual(1);
}

void updateRoomLights(){
  //if light should be on, turn on light
  if(nextRoomLightState == 1 && currentRoomLightState == 0){
    roomLightOn();
  }//if light should be off, turn off light 
  else if(nextRoomLightState == 0 && currentRoomLightState == 1){
    roomLightOff();
  }
}

void roomLightOn(){
    Serial.println("roomLightOn");
    command = "{\"on\":true, \"bri\":" + String(currDimValue) + "}";
    setHue(1,command);
    setHue(2,command);
    currentRoomLightState = 1;
    Blynk.virtualWrite(0, 1);
    Blynk.syncVirtual(0);
}

void roomLightOff(){
    Serial.println("roomLightOff");
    command = "{\"on\": false}";
    setHue(1,command);
    setHue(2,command); 
    currentRoomLightState = 0; 
    Blynk.virtualWrite(0, 0);
    Blynk.syncVirtual(0);
}

void HTTP_GET_REQ(char* host, char* url){
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    return;
  }  
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
   //This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
}

boolean setHue(int lightNum, String command)
{
 Serial.print("setHue: ");
 Serial.println(String(lightNum));
  WiFiClient client;
  if (client.connect(hueHubIP, hueHubPort))
  {
      client.print("PUT /api/");
      client.print(hueUsername);
      client.print("/lights/");
      client.print(lightNum);  // hueLight zero based, add 1
      client.println("/state HTTP/1.1");
      client.println("keep-alive");
      client.print("Host: ");
      client.println(hueHubIP);
      client.print("Content-Length: ");
      client.println(command.length());
      client.println("Content-Type: text/plain;charset=UTF-8");
      client.println();  // blank line before body
      client.println(command);  // Hue command
    client.stop();
    return true;  // command executed
  }
  else
    return false;  // command failed
}


