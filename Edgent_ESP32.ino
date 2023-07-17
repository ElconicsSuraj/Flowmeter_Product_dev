/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/

/* Fill in information from your Blynk Template here */
/* Read more: https://bit.ly/BlynkInject */


// This is code for data logging with timestamp 
// components used SD card module | ESP32 | RTC module 

#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
#define BLYNK_TEMPLATE_ID           "TMPL3jRssuUcA"
#define BLYNK_TEMPLATE_NAME         "Flowmeter"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG



#include "BlynkEdgent.h"
#define BLYNK_PRINT Serial
#include <WiFi.h>

#include <Servo.h>
int SENSOR = 25;

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 0.15;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
File myFile;
const int CS = 5;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}




Servo servo1, servo2, servo3, servo4, servo5;

//char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Elconics"; // Change your WiFi/Hotspot Name
char pass[] = "Elconics@123"; // Change your WiFi/Hotspot Password

int button = 15;

BLYNK_WRITE(V0)
{
  // Function to handle data from virtual pin V0
  int pinValue = param.asInt();
  digitalWrite(2, pinValue);
}

BLYNK_WRITE(V3)
{
  // Function to handle data from virtual pin V3
  int s3 = param.asInt();
  servo4.write(s3);
  Blynk.virtualWrite(V8, s3);
  Serial.println(s3);
}

BLYNK_WRITE(V4)
{
  // Function to handle data from virtual pin V4
  int s4 = param.asInt();
  servo5.write(s4);
  Blynk.virtualWrite(V9, s4);
}

void setup() {
  
  Serial.begin(115200);
  
  // Initialize RTC
  // if (!rtc.begin())
  // {
  //   Serial.println("Couldn't find RTC");
  //   while (1);
  // }

    BlynkEdgent.begin();

  // Set the time if RTC lost power
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power! Setting up time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("Initializing SD card...");
  // if (!SD.begin(CS))
  // {
  //   Serial.println("SD card initialization failed!");
  //   while (1);
  // }
  // Serial.println("SD card initialized.");
    pinMode(2, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(button, INPUT_PULLUP);
  servo4.attach(4);
  servo5.attach(13);
  
  // Uncomment the lines below if you have servo1, servo2, or servo3 attached
  // servo1.attach(15);
  // servo2.attach(16);
  // servo3.attach(17);

 // Blynk.begin(auth, ssid, pass); // Connect to Blynk server
  delay(2000); // Splash screen delay
  pinMode(SENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

}

void loop() {

    DateTime now = rtc.now();
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(" ");

  Serial.print(now.day(), DEC);
  Serial.print(":");
  Serial.print(now.month(), DEC);
  Serial.print(":");
  Serial.print(now.year(), DEC);
  Serial.print("  ");

  Serial.println(daysOfTheWeek[now.dayOfTheWeek()]);
  delay(1000);

  // Generate timestamp and random data
  char timestamp[25];
  sprintf(timestamp, "%02d:%02d:%02d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
  int randomData = random(100);

  // Create message with timestamp and data
  char message[50];
  sprintf(message, "%s - Data: %d", timestamp, randomData);

  // Write message to file
  WriteFile("/test.txt", message);
  delay(100);

  // Read and print file contents
  ReadFile("/test.txt");

    BlynkEdgent.run();
  //int state = digitalRead(button);
  
  // Generate random potentiometer value and send it to virtual pin V1


  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    pulse1Sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    flowMilliLitres = (flowRate / 60) * 1000;

    totalMilliLitres += flowMilliLitres;

    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));
    Serial.print("L/min");
    Serial.print("\t");

    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");

 
  Blynk.virtualWrite(V1, flowRate);
  Blynk.virtualWrite(V4,(totalMilliLitres / 1000));
  }
  
  
  

  delay(100); //Wait for 5 seconds before logging the next data
}




void WriteFile(const char *path, const char *message)
{
  myFile = SD.open(path, FILE_WRITE);
  if (myFile)
  {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  }
  else
  {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

void ReadFile(const char *path)
{
  myFile = SD.open(path);
  if (myFile)
  {
    Serial.printf("Reading file from %s\n", path);
    while (myFile.available())
    {
      Serial.write(myFile.read());
    }
    myFile.close();
  }
  else
  {
    Serial.println("error opening file");
  }
}
