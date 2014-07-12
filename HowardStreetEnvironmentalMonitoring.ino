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
#include "Secrets.h"
#include <avr/wdt.h>

//Define the data formats
//One of "F", "C" or "K"
#define UNIT_FORMAT         'F'
//1 For true, 0 for false
#define DEBUG_STATEMENTS    0
#if DEBUG_STATEMENTS
#define DBG_print(x) Serial.print(x)
#define DBG_println(x) Serial.println(x)
#else
#define DBG_print(x) {}
#define DBG_println(x) {}
#endif

//ThingSpeak will throttle at anything faster than 1 update every 15 seconds
#define UPDATE_RATE         15000

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

//Define the security mode
#define WLAN_SECURITY   WLAN_SEC_WPA2

//Define the TMP006 Sensor Parameters
Adafruit_TMP006 tmp006;

// Define AM2302 DHT22 Parameters
#define DHTPIN 7
#define DHTTYPE DHT22
//Initialize the DHT
DHT dht(DHTPIN, DHTTYPE);

// ThingSpeak Settings
  // Write API Key for a ThingSpeak Channel
#define WEBSITE       "api.thingspeak.com"


// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV4); // you can change this clock speed, and you should...
//Create a client instance                                         
Adafruit_CC3000_Client www;

//Program variables
uint32_t ip = 0;

float floorTemperatureFlt = 0.0f;
float airTemperatureFlt = 0.0f;
float airHumidityFlt = 0.0f;

long startMicros = 0;
long duration = 0;

char floorTemperatureCh[8];
char airTemperatureCh[8];
char airHumidityCh[8];

//Program functions

void sampleFloorTemperature() {
   floorTemperatureFlt = convertToPreferredUnits(tmp006.readObjTempC());
   dtostrf(floorTemperatureFlt, 3, 3, floorTemperatureCh);
}

//The read temperature action reads the last read samp
void sampleAirTemperature() {
  dht.readTemperature();
  delay(10);
  airTemperatureFlt = convertToPreferredUnits(dht.readTemperature());
  dtostrf(airTemperatureFlt, 3, 3, airTemperatureCh);
}

void sampleAirHumidity() {
  dht.readHumidity();
  delay(10);
  dtostrf(airHumidityFlt = dht.readHumidity(), 3, 3, airHumidityCh);
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
  DBG_print(WEBSITE);
  DBG_println(F(" executing domain lookup."));
    if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
      if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
        if (cc3000.getHostByName(WEBSITE, &ip) == 0) {
          DBG_println(F("After three tries the domain name could not be resolved."));
        }
      }
    }
}

//Connects to the AP if the client is not already connected.
void connectAP() {
  DBG_println(F("Begin AP connection."));
  if (!cc3000.begin())
  {
    DBG_println(F("Unable to contact device. Check the module or connection."));
  }
  
  if (!cc3000.checkConnected()) {
    // Connect to WiFi network
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
    DBG_println(F("Connected using AES and WPA2 for security"));
  
    /* Wait for DHCP to complete */
    DBG_println(F("Checking DHCP address allocation."));
    if (!cc3000.checkDHCP())
    {
      //wait a bit longer for the address.
      delay(3000);
    }
    
    if (cc3000.checkDHCP()) {
      DBG_println(F("DHCP address acquired."));
    } else {
      DBG_println(F("DHCP address not acquired!"));
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
  
  DBG_println("\n\n\n");
  // Initialize
  Serial.begin(115200);
  DBG_println(F("TMP006 Initialization Starting."));
  tmp006.begin();
  DBG_println(F("TMP006 Initialization Complete."));
  
  DBG_println(F("TMP006 - Taking initial floor temperature."));
  sampleFloorTemperature();
  DBG_print(floorTemperatureCh);
  DBG_println(UNIT_FORMAT);
  
  DBG_println(F("AM2303/DHT22 Initialization Starting."));
  dht.begin();
  DBG_println(F("AM2303/DHT22 Initialization Complete."));  
  
  DBG_println(F("Taking initial air temperature."));
  sampleAirTemperature();
  DBG_print(airTemperatureCh);
  DBG_println(UNIT_FORMAT);
  
  DBG_println(F("Taking initial air humidity."));
  sampleAirHumidity();
  DBG_print(airHumidityCh);
  DBG_println(F(" %"));

  DBG_println(F("Connecting to the Access Point."));
  connectAP();
  DBG_println(F("Connection Complete."));
  
  DBG_println(F("Resolving the thingspeak address"));
  resolveServerAddress();
  DBG_println(F("Address resolved"));
  
  DBG_print(F("Requests are made with API key: "));
  DBG_println(API_KEY);
}

void loop(void){
  
  delay(10000);
  
  if (DEBUG_STATEMENTS) startMicros = micros();
  sampleAirTemperature();
  sampleAirHumidity();
  sampleFloorTemperature();
  if (DEBUG_STATEMENTS) duration = micros() - startMicros;
  
  DBG_print(F("Data sampling took: "));
  DBG_print(duration/4L);
  DBG_println(F(" uS"));
  
  // Hack and slash String based method of concatenating data for proper format to write to Thingspeak channels
  //requestString = "field1=" + airTemperatureStr + "&field2=" + airHumidityStr + "&field3=" + floorTemperatureStr;

  if (DEBUG_STATEMENTS) startMicros = micros();
  updateThingSpeak();
  if (DEBUG_STATEMENTS) duration = micros() - startMicros;
  
  DBG_print(F("Transmission took: "));
  DBG_print(duration/4L);
  DBG_println(F(" uS"));
  
  DBG_print(F("Free RAM: "));
  DBG_print(freeRam());
  DBG_println(F(" bytes"));
   
  delay(6000);
}

void updateThingSpeak()
{
  wdt_enable(WDTO_2S);
  
  wdt_reset();
  www = cc3000.connectTCP(ip, 80);
  wdt_reset();
  if (www.connected())
  { 
    
    DBG_println(F("Connected to thingspeak. Forming request."));
    wdt_reset();    
    www.fastrprint(F("GET http://api.thingspeak.com/update?key="));
    wdt_reset();
    www.fastrprint(API_KEY);
    wdt_reset();
    www.fastrprint(F("&field1="));
    wdt_reset();
    www.fastrprint(airTemperatureCh);
    wdt_reset();
    www.fastrprint(F("&field2="));
    wdt_reset();
    www.fastrprint(airHumidityCh);
    wdt_reset();
    www.fastrprint(F("&field3="));
    wdt_reset();
    www.fastrprint(floorTemperatureCh);
    wdt_reset();
    www.fastrprint(F("&headers=false"));
    wdt_reset();
    www.fastrprint(floorTemperatureCh);
    wdt_reset();
    www.fastrprint(F(" HTTP/1.1\n"));
    wdt_reset();
    www.fastrprint(F("Host: api.thingspeak.com\n"));
    wdt_reset();
    www.fastrprint(F("Connection: close\n"));
    wdt_reset();
    www.fastrprint(F("\n\n"));
    wdt_reset();
    
    DBG_println(F("Reading response."));
    wdt_reset();    
    while (www.connected()) {
      while (www.available()) {
        wdt_reset();
        char c = www.read();
        DBG_print(c);
      }
    }
    
    DBG_println("");
    DBG_println(F("Response complete."));
    
    wdt_reset();
    www.close();
  }
  else
  {
    wdt_disable();
    DBG_println(F("\nConnection Failed. Attempting to reset the connection.\n"));
    //Refresh the connection
    connectAP();
    resolveServerAddress();
    //Serial.println(F("Looking up server address to be sure it has not changed.")); 
    //resolveServerAddress();
    //Serial.println(F("Server address updated."));
    DBG_println(F("\nReset complete.\n"));
   }
   wdt_disable();
}


