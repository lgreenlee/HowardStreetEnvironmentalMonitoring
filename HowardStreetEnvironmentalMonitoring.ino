// ****************************************************/
// google form key 1xr9hl_o8oKVlNKl1YOUfO7-ZaXEyUgoqb5ApQb9HDUM

// Libraries
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <string.h>
#include "DHT.h"
#include <stdlib.h>
#include <Wire.h>
#include "Adafruit_TMP006.h"

//Define the data formats
//One of "F", "C" or "K"
#define UNIT_FORMAT         'F'
//1 For true, 0 for false
#define DEBUG_STATEMENTS    1
//ThingSpeak will throttle at anything faster than 1 update every 15 seconds
#define UPDATE_RATE         15000

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// WLAN parameters
#define WLAN_SSID       "----"
#define WLAN_PASS       "----"
#define WLAN_SECURITY   WLAN_SEC_WPA2

//Define the TMP006 Sensor Parameters
Adafruit_TMP006 tmp006;

// Define AM2302 DHT22 Parameters
#define DHTPIN 7
#define DHTTYPE DHT22
//Initialize the DHT
DHT dht(DHTPIN, DHTTYPE);

// ThingSpeak Settings
#define API_KEY     "-----"    // Write API Key for a ThingSpeak Channel
#define WEBSITE     "api.thingspeak.com"
uint32_t ip = 0;

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed
//Create a client instance                                         
Adafruit_CC3000_Client www;

//Program variables

float floorTemperatureFlt = 0.0f;
float airTemperatureFlt = 0.0f;
float airHumidityFlt = 0.0f;

long startMicros = 0;
long duration = 0;

char sampleHolderCh[7];
String floorTemperatureStr;
String airTemperatureStr;
String airHumidityStr;

String requestString = "";

//Program functions

void sampleFloorTemperature() {
   floorTemperatureFlt = convertToPreferredUnits(tmp006.readObjTempC());
   dtostrf(floorTemperatureFlt, 3, 3, sampleHolderCh);
   floorTemperatureStr = String(sampleHolderCh);
}

//The read temperature action reads the last read samp
void sampleAirTemperature() {
  airTemperatureFlt = convertToPreferredUnits(dht.readTemperature());
  dtostrf(airTemperatureFlt, 3, 3, sampleHolderCh);
  airTemperatureStr = String(sampleHolderCh);
}

void sampleAirHumidity() {
  dht.readHumidity();
  dtostrf(airHumidityFlt = dht.readHumidity(), 3, 3, sampleHolderCh);
  airHumidityStr = String(sampleHolderCh);
}

float convertToPreferredUnits(float value) {
   
   if (UNIT_FORMAT == 'F') {
     value = convertCtoF(value);
   } else if (UNIT_FORMAT == 'K') {
     value = convertCtoF(value);
   }
   
   return value;
}

//convert celcius to float and ensure the use of float math
float convertCtoF(float value) {
 return value * (9.0f/5.0f) + 32.0f;
}

//convert fahrenheit to celsius and ensure the use of float math
float convertFtoC(float value) {
 return (value - 32.0f) * (5.0f/9.0f);
}

float convertCtoK(float value) {
 return value + 273.15f;
}

void resolveServerAddress() {
 // Try looking up the website's IP address
    Serial.print(WEBSITE); 
    Serial.println(F(" executing domain lookup."));
  
    if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
      if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
        if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
          Serial.println("After three tries the domain name could not be resolved.");
        }
      }
    }
}

//Connects to the AP if the client is not already connected.
void connectAP() {
  Serial.println(F("Begin AP connection."));
  if (!cc3000.begin())
  {
    Serial.println(F("Unable to contact device. Check the module or connection."));
    while(1);
  }
  
  if (!cc3000.checkConnected()) {
    // Connect to WiFi network
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
    Serial.println(F("Connected using AES and WPA2 for security"));
  
    /* Wait for DHCP to complete */
    Serial.println(F("Checking DHCP address allocation."));
    if (!cc3000.checkDHCP())
    {
      //wait a bit longer for the address.
      delay(3000);
    }
    
    if (cc3000.checkDHCP()) {
     Serial.println(F("DHCP address acquired."));
    } else {
      Serial.println(F("DHCP address not acquired!"));
    }
  }
}

//Used from AvailableMemory
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

//Initalize all relevent devices on the board. Ensure that each device can be properly read and report the value. Generally speaking the Floor temp and the air temp should only diverge by a few degrees on startup.
void setup(void){
  
  Serial.println("\n\n\n");
  // Initialize
  Serial.begin(115200);
  Serial.println(F("TMP006 Initialization Starting."));
  tmp006.begin();
  Serial.println(F("TMP006 Initialization Complete."));
  
  Serial.println(F("TMP006 - Taking initial floor temperature."));
  sampleFloorTemperature();
  Serial.print(floorTemperatureStr);
  Serial.println(UNIT_FORMAT);
  
  Serial.println(F("AM2303/DHT22 Initialization Starting."));
  dht.begin();
  Serial.println(F("AM2303/DHT22 Initialization Complete."));  
  
  Serial.println(F("Taking initial air temperature."));
  sampleAirTemperature();
  Serial.print(airTemperatureStr);
  Serial.println(UNIT_FORMAT);
  
  Serial.println(F("Taking initial air humidity."));
  sampleAirHumidity();
  Serial.print(airHumidityStr);
  Serial.println(F(" %"));

  Serial.println(F("Connecting to the Access Point."));
  connectAP();
  Serial.println(F("Connection Complete."));
  
  Serial.println(F("Resolving the thingspeak address"));
  resolveServerAddress();
  Serial.println(F("Address resolved"));
  
  Serial.print(F("Requests are made with API key: "));
  Serial.println(API_KEY);   
}

void loop(void){
  
  startMicros = micros();
  sampleAirTemperature();
  sampleAirHumidity();
  sampleFloorTemperature();
  duration = micros() - startMicros;
  
  Serial.print(F("Data sampling took: "));
  Serial.print(duration/4L);
  Serial.println(F(" uS"));
  
  // Hack and slash String based method of concatenating data for proper format to write to Thingspeak channels
  requestString = "field1=" + airTemperatureStr + "&field2=" + airHumidityStr + "&field3=" + floorTemperatureStr;

  startMicros = micros();
  updateThingSpeak(requestString);
  duration = micros() - startMicros;
  
  Serial.print(F("Transmission took: "));
  Serial.print(duration/4L);
  Serial.println(F(" uS"));
  
  if (DEBUG_STATEMENTS) {
    Serial.println(requestString);
    Serial.print(F("Free RAM: "));
    Serial.print(freeRam());
    Serial.println(F(" bytes"));
  }
   
  delay(UPDATE_RATE);
}

void updateThingSpeak(String tsData)
{
  
  www = cc3000.connectTCP(ip, 80);
  
  if (www.connected())
  { 
    
    if (DEBUG_STATEMENTS) {
          Serial.println(F("Connected to thingspeak. Forming request."));
    }
    
    www.print(F("POST /update HTTP/1.1\n"));
    www.print(F("Host: api.thingspeak.com\n"));
    www.print(F("Connection: close\n"));
    www.print(F("headers=false\n"));
    www.print(F("X-THINGSPEAKAPIKEY: "));
    www.print(API_KEY);
    www.print(F("\n"));
    www.print(F("Content-Type: application/x-www-form-urlencoded\n"));
    www.print(F("Content-Length: "));
    www.print(tsData.length());
    www.print(F("\n\n"));
    www.println(tsData);
    
    Serial.println(F("Reading response."));
    
    while (www.connected()) {
      while (www.available()) {
        char c = www.read();
        if (DEBUG_STATEMENTS) {
          Serial.print(c);
        }
      }
    }
    Serial.println("");
    Serial.println(F("Response complete."));
    www.close();
  }
  else
  {
    Serial.println();
    Serial.println(F("Connection Failed. Attempting to reset the connection."));
    Serial.println();
    //Refresh the connection
    connectAP();
    resolveServerAddress();
    //Serial.println(F("Looking up server address to be sure it has not changed.")); 
    //resolveServerAddress();
    //Serial.println(F("Server address updated."));
    Serial.println();
    Serial.println(F("Reset complete."));
    Serial.println();
   }
}


