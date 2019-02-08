#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>    
#include <Encoder.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

Encoder myEnc(D5, D4);    
long oldPosition  = -999;
bool connectedToWifi = false;

const char* mqtt_server = "68.183.121.10";

#define MQTT_PORT 1883

#define MQTT_CLIENT_NAME "JKFam1_Ven"

uint32_t currentColor = 0xFF00FF;
#define LIGHT_ON_DURATION 1000

#define IN_TOPIC "KeithFamily"
#define OUT_TOPIC "KeithFamily"

#define INPUT_PIN D3

// Setup LEDs
#define NUMPIXELS 255 
#define DATAPIN    4
#define CLOCKPIN   5
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long lightStartedTime = 0;
int lastPosition = 0;

void setup() {
  Serial.begin(115200);

  setup_wifi_managed();

  connectToMQTT();  

  pinMode(INPUT_PIN, INPUT_PULLUP);

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP

  setInitialColor();

}

void setup_wifi_managed() {

  Serial.println();
  Serial.print("Launching Wifi Manager");

  WiFiManager wifiManager;
  wifiManager.autoConnect("Friendship Venus", "friendship");

  handleWifiStatus();

}

void setInitialColor() {

  if (!connectedToWifi) {
    int val = 340 % 255 ;
    myEnc.write(val);
    lastPosition = val;
    currentColor = Wheel(lastPosition%255);
  } else {
    myEnc.write(0);
    lastPosition = 0;
    currentColor = Wheel(0);
  }
}

void connectToMQTT() {
  if (connectedToWifi) {
    client.setServer(mqtt_server, MQTT_PORT);
    client.setCallback(callback);
  } else {
    Serial.println("Skipping MQTT Connection and running in local mode");
  }
}



void handleWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    connectedToWifi = true;
  } else {
    Serial.println("");
    Serial.println("WiFi failed to connect");
    connectedToWifi = false;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  myEnc.write(payload[0]);

  lightStartedTime = millis();

}

void reconnect() {

  if (connectedToWifi) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_NAME)) {
      Serial.println("connected");
      client.subscribe(IN_TOPIC);
    } 
    else 
    {
      Serial.println("Failed MQTT Connection");
    }
  }
}

void clearAllPixels()
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    strip.setPixelColor(i, 0x000000);
  }
}

void lightMainPixelColor()
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    strip.setPixelColor(i, currentColor);
  }
}

void checkMQTTChannel() 
{
  if (!client.connected()) 
  {
    reconnect();
  } else {
    client.loop();
  }
}

void loop() {

  long newPosition = myEnc.read();

  if (newPosition != lastPosition) {
    currentColor = Wheel(newPosition%255);
  }
  
  checkMQTTChannel();

  if(digitalRead(INPUT_PIN) == 0)
  {
    msg[0] = (char)newPosition%255;
    client.publish(OUT_TOPIC, msg);
  }

  lightMainPixelColor();

  strip.show();
}