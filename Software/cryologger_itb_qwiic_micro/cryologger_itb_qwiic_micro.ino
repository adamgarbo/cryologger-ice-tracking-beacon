/*
    Title:    Cryologger Ice Tracking Beacon (ITB) - Version 3
    Date:     November 13, 2020
    Author:   Adam Garbo

    Components:
    - SparkFun Qwiic Micro - SAMD21 Development Board
    - SparkFun Atmospheric Sensor Breakout - BME280 (Qwiic)
    - SparkFun GPS Breakout - SAM-M8Q (Qwiic)
    - SparkFun Qwiic Iridium 9603N
    - Maxtena M1621HCT-P-SMA Iridium antenna

    - SparkFun Buck-Boost Converter

    Comments:
    - Code is currently under development for the next iteration of the Cryologger iceberg
    tracking beacon to be deployed during the 2021 Amundsen Expedition.
    - The MicroMod system has potential, but it's really a pain in the ass.
    - With the addition of the Qwiic Power Switch and the Buck-Boost Converter feeding the
    3.3V bus directly, we're looking at 110-141 uA in sleep mode (total) at 7.4-12V.
    - Artemis MicroMod is currently sitting at 300 uA.
*/

// Libraries
#include <Wire.h>                           // https://www.arduino.cc/en/Reference/Wire
#include <SPIMemory.h>                      // https://github.com/Marzogh/SPIMemory
#include <SparkFun_Ublox_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library
#include <SparkFun_Qwiic_Power_Switch_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_Qwiic_Power_Switch_Arduino_Library
#include <SparkFunBME280.h>                 // https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
#include <RTCZero.h>                        // https://github.com/arduino-libraries/RTCZero
#include <IridiumSBD.h>                     // https://github.com/sparkfun/SparkFun_IridiumSBD_I2C_Arduino_Library
#include <ArduinoLowPower.h>                // https://github.com/arduino-libraries/ArduinoLowPower

// Defined constants
#define Serial        SerialUSB   // Required by SparkFun Qwiic Micro 

#define DEBUG         true        // Output debugging messages to Serial Monitor
#define DIAGNOSTICS   false        // Output Iridium diagnostic messages to Serial Monitor

// Object instantiations
BME280        bme280;             // I2C Address: 0x77
IridiumSBD    modem(Wire);        // I2C Address: 0x63
QWIIC_POWER   mySwitch;           // I2C Address: 0x41
RTCZero       rtc;
SFE_UBLOX_GPS gps;                // I2C Address: 0x42
//SPIFlash      flash(21, &SPI1);

// User defined global variable declarations
unsigned long alarmInterval         = 60;     // RTC sleep duration in seconds (Default: 3600 seconds)
byte          alarmSeconds          = 0;
byte          alarmMinutes          = 1;
byte          alarmHours            = 0;
byte          transmitInterval      = 1;     // Number of messages to transmit in each Iridium transmission (340 byte limit)
byte          maxRetransmitCounter  = 1;      // Number of failed data transmissions to reattempt (340 byte limit)

// Global variable and constant declarations
volatile bool alarmFlag             = false;  // Flag for alarm interrupt service routine
volatile bool watchdogFlag          = false;  // Flag for Watchdog Timer interrupt service routine
volatile int  watchdogCounter       = 0;      // Watchdog Timer interrupt counter
bool          ledState              = LOW;    // Flag to toggle LED in blinkLed() function
bool          rtcSyncFlag           = true;   // Flag to determine if RTC should be set using GNSS time
bool          resetFlag             = 0;      // Flag to force system reset using Watchdog Timer

int           valFix                = 0;      // GNSS valid fix counter
int           maxValFix             = 5;      // Max GNSS valid fix counter

uint8_t       transmitBuffer[340] = {};       // Qwiic Iridium 9603N transmission buffer
unsigned int  messageCounter      = 0;        // Qwiic Iridium 9603N transmitted message counter
unsigned int  retransmitCounter   = 0;        // Qwiic Iridium 9603N failed data transmission counter
unsigned int  transmitCounter     = 0;        // Qwiic Iridium 9603N transmission interval counter

unsigned long previousMillis      = 0;        // Global millis() timer

time_t        alarmTime           = 0;
time_t        unixtime            = 0;

// Union to store and send data byte-by-byte via Iridium
typedef union {
  struct {
    uint32_t  unixtime;           // UNIX Epoch time                (4 bytes)
    int32_t   latitude;           // Latitude (DD)                  (4 bytes)
    int32_t   longitude;          // Longitude (DD)                 (4 bytes)
    uint8_t   satellites;         // # of satellites                (1 byte)
    uint8_t   pdop;               // PDOP                           (1 byte)
    uint8_t   fix;                // Fix                            (1 byte)
    uint16_t  transmitDuration;   // Previous transmission duration (2 bytes)
    uint16_t  messageCounter;     // Message counter                (2 bytes)
  } __attribute__((packed));                                        // Total: (19 bytes)
  uint8_t bytes[19];
} SBDMESSAGE;

SBDMESSAGE message;
size_t messageSize = sizeof(message);   // Size (in bytes) of message to be transmitted

// Devices that may be online or offline.
struct struct_online {
  bool iridium = false;
  bool gnss = false;
} online;

// Setup
void setup() {

  // Pin assignments
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Wire.begin(); // Initialize I2C
  //SPI1.begin(); // Initialize SPI

  Serial.begin(115200);
  //while (!Serial); // Wait for user to open Serial Monitor
  delay(5000); // Delay to allow user to open Serial Monitor

  printLine();
  Serial.println(F("Cryologger Iceberg Tracking Beacon"));
  printLine();

  // Configure
  configureWdt();         // Configure Watchdog Timer
  configureQwiicPower();  // Configure Qwiic Power Switch
  configureRtc();         // Configure real-time clock (RTC)
  configureGnss();        // Configure Sparkfun SAM-M8Q
  configureIridium();     // Configure SparkFun Qwiic Iridium 9603N
  syncRtc();              // Synchronize RTC with GNSS

  Serial.flush(); // Wait for transmission of any serial data to complete
}

// Loop
void loop() {

  // Check if alarm flag was set
  if (alarmFlag) {
    alarmFlag = false;  // Clear alarm flag

    // Perform measurements
    readRtc();          // Read RTC
    petDog();           // Pet the Watchdog Timer
    //readSensors();      // Read sensors
    readGnss();         // Read GNSS
    writeBuffer();      // Write data to buffer
    transmitData();     // Transmit data
    setRtcAlarm();      // Set RTC alarm
  }

  // Check for watchdog interrupt
  if (watchdogFlag) {
    petDog();
  }

  // Blink LED
  blinkLed(1, 100);

  // Enter deep sleep and await RTC alarm interrupt
  goToSleep();
}
