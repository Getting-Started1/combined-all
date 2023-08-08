
#ifdef ENABLE_DEBUG
   #define DEBUG_ESP_PORT Serial
   #define NODEBUG_WEBSOCKETS
   #define NDEBUG
#endif 

#include <Arduino.h>
#ifdef ESP8266 
   #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
   #include <WiFi.h>
#endif

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include "twilio.hpp"

#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Values from Twilio (find them on the dashboard)
static const char *account_sid = "AC03448c9af4e928a5bd9ec4ee8a80b369";
static const char *auth_token = "11d46fe928721150ed083655eef7b498";

// Phone number should start with "+<countrycode>"
//You will have to sign up for the twillio account so they can give you a number
static const char *from_number = "<+15418735775>";

// Whatsapp number and Api key
//Your mobile number and the apiKey from whatsapp bot
// to install the whatsapp bot whatsapp this number 
// +34 644 51 95 23
//send whatsapp message "I allow callmebot to send me messages in order to get APIKey"
String phoneNumber = "+254797653523";
String apiKey = "3389715";

const int motionSensor = 39; // PIR Motion Sensor
bool motionDetected = false;

//gas pin for the mq2 sensor
const int GAS_PIN = 20;
unsigned long previousMillisMQ2 = 0; // Variable to store the previous time
unsigned long intervalMQ2 = 10000; // Interval between gas level readings in milliseconds


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define DHT_SENSOR_TYPE DHT22
#define DHT_PIN 7 // Replace with your desired pin

DHT dht_sensor(DHT_PIN, DHT_SENSOR_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// In order to have access for the lights visit sinricPro create new devices add 3 new devices under the lights section
// Copy the APP_KEY, APP_SECRET, SWITCH_IDS
#define WIFI_SSID         "Elly"
#define WIFI_PASS         "987654321"
#define APP_KEY           "4bfaf63c-dbd5-4603-bfce-a1e4b991462b"
#define APP_SECRET        "d2170a7d-a600-4698-ae86-8e8be160a9ea-a3423086-cee5-4dc8-a245-48cb630471ea"

#define SWITCH_ID_1       "64d1e44a708e0f2583b86e43"
#define RELAYPIN_1        16

#define SWITCH_ID_2       "64d1e532a1c7182622ab83f7"
#define RELAYPIN_2        19

#define SWITCH_ID_3       "64d1e56a708e0f2583b86ebb"
#define RELAYPIN_3        33

#define BAUD_RATE         115200   


// You choose!
// Phone number should start with "+<countrycode>"
static const char *to_number = "<+254797653523>";
static const char *message = "The Temperature has risen above 25 (via twilio)";

Twilio *twilio;

unsigned long previousMillis = 0;
const unsigned long interval = 2000; // Interval between readings, in milliseconds

void IRAM_ATTR detectsMovement() {
  //Serial.println("MOTION DETECTED!!!");
  motionDetected = true;
}

void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_1, state ? HIGH:LOW);
 return true; // request handled properly
}

bool onPowerState2(const String &deviceId, bool &state) {
 Serial.printf("Device 2 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_2, state ? HIGH:LOW);
 return true; // request handled properly
}

bool onPowerState3(const String &deviceId, bool &state) {
 Serial.printf("Device 3 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_3, state ? HIGH:LOW);
 return true; // request handled properly
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }

  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() {
  // add devices and callbacks to SinricPro
  pinMode(RELAYPIN_1, OUTPUT);
  pinMode(RELAYPIN_2, OUTPUT);
  pinMode(RELAYPIN_3, OUTPUT);
    
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  
  SinricProSwitch& mySwitch2 = SinricPro[SWITCH_ID_2];
  mySwitch2.onPowerState(onPowerState2);
  
  SinricProSwitch& mySwitch3 = SinricPro[SWITCH_ID_3];
  mySwitch3.onPowerState(onPowerState3);
  
  
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
   
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");

  Wire.begin(42,2);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  dht_sensor.begin();
  setupWiFi();
  setupSinricPro();

  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  //pinmode for the MQ2
  pinMode(GAS_PIN, INPUT);
  twilio = new Twilio(account_sid, auth_token);
}

void loop() {
  SinricPro.handle();
  if(motionDetected){
    sendMessage("Motion detected!!");
    Serial.println("Motion Detected");
    motionDetected = false;
  }
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    display.clearDisplay();
    float humi = dht_sensor.readHumidity();
    float tempC = dht_sensor.readTemperature();

    if(tempC>25){
      digitalWrite(38, LOW);
      digitalWrite(35, HIGH);
    }else{
      digitalWrite(38, LOW);
      digitalWrite(35, LOW);
    }

    if (isnan(tempC) || isnan(humi)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Temperature: ");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(tempC);
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);
      display.write(167);
      display.setTextSize(2);
      display.print("C");

      display.setTextSize(1);
      display.setCursor(0, 35);
      display.print("Humidity: ");
      display.setTextSize(2);
      display.setCursor(0, 45);
      display.print(humi);
      display.print(" %");

      display.display();
    }
  }
  unsigned long currentMillisMQ2 = millis(); // Get the current time

  if (currentMillisMQ2 - previousMillisMQ2 >= intervalMQ2) {
    previousMillisMQ2 = currentMillisMQ2; // Save the current time

    int gasLevel = analogRead(GAS_PIN);

    Serial.print("Gas Level: ");
    Serial.print(gasLevel);
    Serial.print("\t");
    Serial.print("\t");


    if (gasLevel > 1000) {
      Serial.println("Gas");
      String response;
      bool success = twilio->send_message(to_number, from_number, message, response);
      if (success) {
        Serial.println("Sent message successfully!");
      } else {
        Serial.println(response);
      }

    }else{
      Serial.println("No gas");
    }

  }
}
