#include <TridentTD_LineNotify.h>
#define SECRET_WRITE_APIKEY "HYI2HF7TLDM6WENK"
#define LINE_TOKEN "ocZfZpAGweUxe1C7BcqetQ30HKUmihFLr3LYUSQT4CD" // ป้อนรหัส token
///------------------------------------------------
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h>  // Library for LCD
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3F, 16, 2);
///------------------------------------------------
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <HTTPClient.h>
///------------------------------------------------
#define BLYNK_TEMPLATE_ID "TMPL6E6sNQrKC"
#define BLYNK_TEMPLATE_NAME "Fan control"
#define BLYNK_AUTH_TOKEN "2x6qeMZtxN7K8tst6Wh0cCYzpSzoh_tU"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <math.h>
///-----------------------------------------------
const char *ssid = "Free Wifi";
const char *pass = "3.141598";

const char* serverName = "http://api.thingspeak.com/update";
String apiKey = "HYI2HF7TLDM6WENK";
int limit_temp = 30;
int limit_gas =  4095;
WiFiClient client;

#define BMP_CS 5
Adafruit_BMP280 bmp(BMP_CS);
#define Relay_Pin 4
#define GasSensor_Pin 35

///-----------------------------------Blynk part-----------------------------------------
int mode = 1;

BlynkTimer timer;

// Function to control the relay based on button state
BLYNK_WRITE(V0)
{
  int state = param.asInt();
  if (mode == 0) { // Only control relay pin if in Manual Mode
    digitalWrite(Relay_Pin, state); // Set the new state
    Blynk.virtualWrite(V3, digitalRead(Relay_Pin));
    delay(250); // Delay for stability
  }
}


// Function to handle mode selection
BLYNK_WRITE(V1)
{
  int selectedMode = param.asInt();
  if(selectedMode == 1 || selectedMode == 0) {
    mode = selectedMode;
    //----------------Change mode!!! Relay pin set to LOW--------------------------------------------------------------//
    digitalWrite(Relay_Pin, LOW);
  }
}
//----------------------------------------------"httpPostToThingSpeak()"--------------------------------------------------------------//
void httpPostToThingSpeak(unsigned int fieldNumber,float value) {
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Data to send with HTTP POST
  String httpRequestData = "api_key=" + apiKey + "&field" + String(fieldNumber) + "=" + String(value);

  // Send HTTP POST request
  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode == 200) {
    Serial.println("Channel update successful.");
  }
  else if (httpResponseCode == -1) {
    Serial.println("Channel update failed.");
  }

  // Free resources
  http.end();
}

//----------------------------------------------"void setup()","void loop()"--------------------------------------------------------------//
void setup() {
  WiFi.begin(ssid, pass);
  Serial.printf("WiFi connecting ", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());
  LINE.setToken(LINE_TOKEN);

  lcd.begin();
  lcd.backlight();
  lcd.clear();

  pinMode(Relay_Pin, OUTPUT);
  pinMode(GasSensor_Pin, INPUT);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Serial.begin(9600);
  delay(1000);
  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

unsigned int HalfSeconds = 0;

void loop() {
  // Serial.print("Mode: ");
  // Serial.println(mode);

    Serial.print("Temperature = ");
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    int GassensorValue = analogRead(GasSensor_Pin);
    Serial.print("Gas sensor value: ");
    Serial.println(GassensorValue);
    delay(500);
   Serial.println("------------------------------------------------------");

  bool notify = false; // Flag to indicate whether to send notification

  if (mode == 1) {
    if (bmp.readTemperature() > limit_temp && GassensorValue == limit_gas) {
      digitalWrite(Relay_Pin, HIGH); // Set relay pin to high
      notify = true;
    }
    else if (bmp.readTemperature() > limit_temp) {
      digitalWrite(Relay_Pin, HIGH); // Set relay pin to high
      notify = true;
    }
    else if (GassensorValue == limit_gas) {
      digitalWrite(Relay_Pin, HIGH); // Set relay pin to high
      notify = true;
    }
    else {
      digitalWrite(Relay_Pin, LOW); // Set relay pin to low
    }
  }

  // Send notification if needed
  if (notify) {
    Serial.print("Temperature = ");
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    Serial.print("Gas sensor value: ");
    Serial.println(GassensorValue);
    Serial.print("Fan Status : ");
    Serial.println(digitalRead(Relay_Pin));
    delay(5000);
    LINE.notify("Alert!!!!\nTemperature: " + String(bmp.readTemperature(), 3) + "°C\nGas Sensor: " + (GassensorValue == 4095 ? "High gas density level" : "Low gas density level") + "\nFan Status: " + (digitalRead(Relay_Pin) == HIGH ? "ON" : "OFF"));
  }
  else {
    Serial.print("Fan Status : ");
    Serial.println(digitalRead(Relay_Pin));
  }
  
  // Check if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    //Post http get to ThingSpeak Every 15 seconds
    if(HalfSeconds >= 15) {
      //Reset Timer
      HalfSeconds = 0;
      //Write temp To Field 1
      httpPostToThingSpeak(1,bmp.readTemperature());
    }
  }
  else {
    //Reconnecting to Router
    Serial.print("Connecting to :");
    Serial.print(ssid);
    Serial.print(" : ");
    WiFi.begin(ssid, pass);//Connect ESP32 to home network
    while (WiFi.status() != WL_CONNECTED) { //Wait until Connection is complete
      delay(500);
      Serial.print(".");
    }
  }

  //Update Half seconds every 500 ms
  delay(500);
  HalfSeconds++;
  //----
  Blynk.virtualWrite(V4, GassensorValue );  
  Blynk.virtualWrite(V3, digitalRead(Relay_Pin));  
  Blynk.virtualWrite(V2, bmp.readTemperature());  
  //---
  lcd.clear();
  float t = bmp.readTemperature();
  lcd.backlight();
  lcd.print("BMP280 Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Temp : "+ String(t) );

  Blynk.run();
}
