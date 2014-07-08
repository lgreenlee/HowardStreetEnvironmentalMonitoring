DavisThermo
===========

Davis Square Temperature Monitoring

Getting Started

Clone this repository to the folder of your choice on your local machine, this will be [Project Path] for future reference.

Download the Arduino development environment if you have not already done so.

Open the DavisThermo.ino sketch.

Add the dependent libraries to your sketch so it will compile. The libraies are located in [Project Path]\Adafruit_CC3000_Library, [Project Path]\Adafruit_TMP006_Library and [Project Path]\DHT11_Library. These can be added to the sketch by navigating to "Sketch->Import Library->Add Library..." within the Arduino IDE and adding each of the three folders to the libraries.

Hardware Configuration

The board used for this project is a Arduino DUO with the Adafruit Wifi shield. The DHT11 is located on pin 7 of the digital I/O bank and uses 3V-5V for power. The TMP006 is connected via I2C and requires 2.2V-5V
