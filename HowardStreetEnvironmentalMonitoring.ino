// ****************************************************/
// google form key 1xr9hl_o8oKVlNKl1YOUfO7-ZaXEyUgoqb5ApQb9HDUM

// Libraries
#include <Adafruit_CC3000.h>
#include <utility/sntp.h>
#include <SPI.h>
#include "DHT.h"
#include <stdlib.h>
#include <Wire.h>
#include "Adafruit_TMP006.h"
#include "Secrets.h"
#include <avr/wdt.h>

// feature flags
#define USE_NTP 1
#define USE_THINGSPEAK 0
#define USE_GRAPHITE 1

//Define the data formats
//One of "F", "C" or "K"
#define UNIT_FORMAT         'F'
//1 For true, 0 for false
#define DEBUG_STATEMENTS    1
#if DEBUG_STATEMENTS
#define DBG_print(x) Serial.print(x)
#define DBG_println(x) Serial.println(x)
#else
#define DBG_print(x)
#define DBG_println(x)
#endif

#if DEBUG_STATEMENTS && USE_NTP
#define DBG_printTime(t) printTime(t)
#else
#define DBG_printTime(t)
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
#define THINGSPEAK_WEBSITE "api.thingspeak.com"

// Graphite settings
#define GRAPHITE_HOST "test.samgerstein.net"
#define GRAPHITE_BASE "howard-st"

#if USE_GRAPHITE
#define DATA_HOST GRAPHITE_HOST
#else
#define DATA_HOST THINGSPEAK_WEBSITE
#endif

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV4); // you can change this clock speed, and you should...

#if USE_NTP
// NTP support from CC3300; time values are EST / EDT
sntp ntp_client = sntp(NULL, "time.nist.gov", (short)(-5 * 60), (short)(-4 * 60), true);
SNTP_Timestamp_t last_update_time;
// We'll update from an NTP server periodically so as not to drift:
#define UPDATE_CLOCK_PERIOD (60 * 60 * 12)
// NTP doesn't use Unix epoch time, it starts with 1900..
#define NTP_TO_UNIX_SECS (((1970 - 1900) * 365 + 17) * 86400)
#endif (USE_NTP)

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
     value = convertCtoK(value);
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
  DBG_print(F("lookup "));
  DBG_println(DATA_HOST);

  ip = 0;
  while (ip == 0) {
    cc3000.getHostByName(DATA_HOST, &ip);
    if (ip == 0) {
      DBG_println(F("DNS fail.. retry"));
      delay(500);
    }
  }
}

//Connects to the AP if the client is not already connected.
void connectAP() {
  DBG_println(F("Try AP.."));
  if (!cc3000.begin())
  {
    DBG_println(F("Check WiFi!"));
  }
  
  if (!cc3000.checkConnected()) {
    // Connect to WiFi network
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
    DBG_println(F("Connected"));
  
    /* Wait for DHCP to complete */
    DBG_println(F("Check DHCP.."));
    if (!cc3000.checkDHCP())
    {
      //wait a bit longer for the address.
      delay(3000);
    }
    
    if (cc3000.checkDHCP()) {
      DBG_println(F("DHCP success!"));
    } else {
      DBG_println(F("DHCP fail!"));
    }
  }
}

#if USE_NTP
void updateTime(){
  ntp_client.UpdateNTPTime();
  ntp_client.NTPGetTime(&last_update_time, true);
  DBG_printTime(&last_update_time);
}

void printTime(SNTP_Timestamp_t *sometime){
  NetTime_t timeExtract;
  ntp_client.ExtractNTPTime(sometime, &timeExtract);
  Serial.print(timeExtract.hour); Serial.print(F(":")); Serial.print(timeExtract.min); Serial.print(F(":"));Serial.print(timeExtract.sec); Serial.print(F("."));Serial.println(timeExtract.millis);
}

uint32_t currentUnixTime(){
  SNTP_Timestamp_t now;
  ntp_client.NTPGetTime(&now, false);
  return now.seconds - NTP_TO_UNIX_SECS;
}

void periodicUpdateTime(){
    SNTP_Timestamp_t now;
    ntp_client.NTPGetTime(&now, true);
    if (now.seconds - last_update_time.seconds > UPDATE_CLOCK_PERIOD) {
      updateTime();
    }
}
#endif

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
  DBG_println(F("Init TMP006.."));
  tmp006.begin();
  DBG_println(F("OK."));
  
  DBG_println(F("Read floor temp."));
  sampleFloorTemperature();
  DBG_print(floorTemperatureCh);
  DBG_println(UNIT_FORMAT);
  
  DBG_println(F("Init AM2303.."));
  dht.begin();
  DBG_println(F("OK."));  
  
  DBG_println(F("Read air temp."));
  sampleAirTemperature();
  DBG_print(airTemperatureCh);
  DBG_println(UNIT_FORMAT);
  
  DBG_println(F("Read air H2O."));
  sampleAirHumidity();
  DBG_print(airHumidityCh);
  DBG_println(F(" %"));

  connectAP();
  
  resolveServerAddress();
  
  DBG_print(F("API key: "));
  DBG_println(API_KEY);

#if USE_NTP
  DBG_println(F("Setting time"));
  updateTime();
#endif
}

void loop(void){
  
  delay(10000);
  if (USE_NTP) periodicUpdateTime();
  
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
  if (USE_THINGSPEAK)   updateThingSpeak();
  if (USE_GRAPHITE)     updateGraphite();
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
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  wdt_reset();
  if (www.connected())
  { 
    
    DBG_println(F("Connected to thingspeak."));
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
    DBG_println(F("\nConnection Failed. Attempting reset.\n"));
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

void updateGraphite() {
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 2003);
  const char *metrics[][2] = {{"air_temp", airTemperatureCh},
                           {"air_h20", airHumidityCh},
                           {"floor_temp", floorTemperatureCh}
                          };
  char now[12];
  sprintf(now, "%lu", currentUnixTime());

  for (int i=0; i < 3; ++i) {
    client.fastrprint(GRAPHITE_BASE);
    client.fastrprint(".");
    // set GRAPHITE_HOME (eg. in your Secrets.h) to your unit number, like "20-2"
    client.fastrprint(GRAPHITE_HOME);
    client.fastrprint(".");
    client.fastrprint(metrics[i][0]);
    client.fastrprint(" ");
    client.fastrprint(metrics[i][1]);
    client.fastrprint(" ");
    client.fastrprintln(now);
  }
  delay(100);
  client.close();
}

