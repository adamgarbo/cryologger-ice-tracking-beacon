/*
    Title:    Cryologger Ice Tracking Beacon (ITB) - v3.0
    Date:     April 10, 2021
    Author:   Adam Garbo

    Description:
    - Code is currently under development for the next iteration of the
    Cryologger iceberg tracking beacon to be deployed during the 2021
    Amundsen Expedition.

    Components:
    - SparkFun Qwiic Micro - SAMD21 Development Board
    - SparkFun Real Time Clock Module - RV-8803 (Qwiic)
    - SparkFun Atmospheric Sensor Breakout - BME280 (Qwiic)
    - SparkFun GPS Breakout - SAM-M8Q (Qwiic)
    - SparkFun 9DoF IMU Breakout - ICM-20948 (Qwiic)
    - Rock7 RockBLOCK 9603
    - Maxtena M1621HCT-P-SMA Iridium antenna
    - NDP6020P P-Channel MOSFET
    - SparkFun Buck-Boost Converter
*/

// ------------------------------------------------------------------------------------------------
// Libraries
// ------------------------------------------------------------------------------------------------
#include <FastLED.h>                              // https://github.com/adafruit/Adafruit_NeoPixel
#include <ArduinoLowPower.h>                      // https://github.com/arduino-libraries/ArduinoLowPower
#include <ICM_20948.h>                            // https://github.com/sparkfun/SparkFun_ICM-20948_ArduinoLibrary
#include <IridiumSBD.h>                           // https://github.com/sparkfun/SparkFun_IridiumSBD_I2C_Arduino_Library
#include <SAMD_AnalogCorrection.h>                // https://github.com/arduino/ArduinoCore-samd/tree/master/libraries/SAMD_AnalogCorrection
#include <SparkFunBME280.h>                       // https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
#include <SparkFun_RV8803.h>                      // https://github.com/sparkfun/SparkFun_RV-8803_Arduino_Library
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
#include <TimeLib.h>                              // https://github.com/PaulStoffregen/Time
#include <Wire.h>                                 // https://www.arduino.cc/en/Reference/Wire

// ------------------------------------------------------------------------------------------------
// Debugging macros
// ------------------------------------------------------------------------------------------------
#define DEBUG           true   // Output debug messages to Serial Monitor
#define DEBUG_GNSS      false   // Output GNSS debug information
#define DEBUG_IRIDIUM   false   // Output Iridium debug messages to Serial Monitor

#if DEBUG
#define DEBUG_PRINT(x)            SERIAL_PORT.print(x)
#define DEBUG_PRINTLN(x)          SERIAL_PORT.println(x)
#define DEBUG_PRINT_HEX(x)        SERIAL_PORT.print(x, HEX)
#define DEBUG_PRINTLN_HEX(x)      SERIAL_PORT.println(x, HEX)
#define DEBUG_PRINT_DEC(x, y)     SERIAL_PORT.print(x, y)
#define DEBUG_PRINTLN_DEC(x, y)   SERIAL_PORT.println(x, y)
#define DEBUG_WRITE(x)            SERIAL_PORT.write(x)

#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT_HEX(x)
#define DEBUG_PRINTLN_HEX(x)
#define DEBUG_PRINT_DEC(x, y)
#define DEBUG_PRINTLN_DEC(x, y)
#define DEBUG_WRITE(x)
#endif

// ------------------------------------------------------------------------------------------------
// Port definitions
// ------------------------------------------------------------------------------------------------
#define SERIAL_PORT     SerialUSB
#define IRIDIUM_PORT    Serial

// ------------------------------------------------------------------------------------------------
// Pin definitions
// ------------------------------------------------------------------------------------------------
#define PIN_VBAT            A0
#define PIN_IRIDIUM_EN      3
#define PIN_IRIDIUM_SLEEP   4
#define PIN_RTC_INT         5
#define PIN_GNSS_EN         6
#define PIN_LED             7

// ------------------------------------------------------------------------------------------------
// Object instantiations
// ------------------------------------------------------------------------------------------------
CRGB              led[1];
BME280            bme280;         // I2C Address: 0x77
ICM_20948_I2C     imu;            // I2C Address: 0x69
IridiumSBD        modem(IRIDIUM_PORT, PIN_IRIDIUM_SLEEP); // D16 (TX): Pin 1 (yellow) D17 (RX): Pin 6 (orange)
RV8803            rtc;            // I2C Address: 0x32
SFE_UBLOX_GNSS    gnss;           // I2C Address: 0x42

// ------------------------------------------------------------------------------------------------
// User defined global variable declarations
// ------------------------------------------------------------------------------------------------
unsigned long alarmInterval         = 120;    // Sleep duration in seconds
byte          alarmMinutes          = 2;      // Rolling alarm mintues
byte          alarmHours            = 0;      // Rolling alarm hours
byte          alarmDate             = 0;      // Rolling alarm days
unsigned int  transmitInterval      = 1;      // Number of messages to transmit in each Iridium transmission (340 byte limit)
unsigned int  retransmitCounterMax  = 10;      // Number of failed data transmissions to reattempt (340 byte limit)
unsigned int  gnssTimeout           = 1;      // Timeout for GNSS signal acquisition
unsigned int  iridiumTimeout        = 30;     // Timeout for Iridium transmission (s)
unsigned long ledDelay              = 2000;   // Duration of RGB LED colour change (ms)

// ------------------------------------------------------------------------------------------------
// Global variable declarations
// ------------------------------------------------------------------------------------------------
const float   R1                    = 9973000.0;   // Voltage divider resistor 1
const float   R2                    = 998700.0;    // Voltage divider resistor 2
volatile bool alarmFlag             = true;   // Flag for alarm interrupt service routine
volatile bool watchdogFlag          = false;  // Flag for Watchdog Timer interrupt service routine
volatile int  watchdogCounter       = 0;      // Watchdog Timer interrupt counter
bool          firstTimeFlag         = true;   // Flag to determine if the program is running for the first time
bool          resetFlag             = 0;      // Flag to force system reset using Watchdog Timer
float         voltage               = 0.0;    // Battery voltage
uint8_t       transmitBuffer[340]   = {};     // Iridium 9603 transmission buffer (MO SBD message max length: 340 bytes)
unsigned int  messageCounter        = 0;      // Iridium 9603 transmission counter (zero indicates a reset)
byte          retransmitCounter     = 0;      // Iridium 9603 failed transmission counter
byte          transmitCounter       = 0;      // Iridium 9603 transmission interval counter
unsigned int  failedTransmitCounter = 0;
unsigned long previousMillis        = 0;      // Global millis() timer
unsigned long powerDelay            = 2500;   // Delay after power to MOSFET is enabled
time_t        alarmTime, unixtime   = 0;      // Global RTC time variables

// ------------------------------------------------------------------------------------------------
// Data transmission unions/structures
// ------------------------------------------------------------------------------------------------
// Union to transmit Iridium Short Burst Data (SBD) Mobile Originated (MO) message
typedef union
{
  struct
  {
    uint32_t  unixtime;         // UNIX Epoch time                (4 bytes)
    int16_t   temperature;      // Temperature (°C)               (2 bytes)
    uint16_t  humidity;         // Humidity (%)                   (2 bytes)
    uint16_t  pressure;         // Pressure (Pa)                  (2 bytes)
    int32_t   latitude;         // Latitude (DD)                  (4 bytes)
    int32_t   longitude;        // Longitude (DD)                 (4 bytes)
    uint8_t   satellites;       // # of satellites                (1 byte)
    uint16_t  pdop;             // PDOP                           (2 bytes)
    int16_t   rtcDrift;         // RTC offset from GNSS time      (2 bytes)
    uint16_t  voltage;          // Battery voltage (V)            (2 bytes)
    uint16_t  transmitDuration; // Previous transmission duration (2 bytes)
    uint16_t  messageCounter;   // Message counter                (2 bytes)
  } __attribute__((packed));                              // Total: (29 bytes)
  uint8_t bytes[29];
} SBD_MO_MESSAGE;

SBD_MO_MESSAGE moMessage;

// Union to receive Iridium SBD Mobile Terminated (MT) message
typedef union
{
  struct
  {
    uint32_t  alarmInterval;      // 4 bytes
    uint8_t   transmitInterval;   // 1 byte
    uint8_t   retransmitCounter;  // 1 byte
    uint8_t   resetFlag;          // 1 byte
  };
  uint8_t bytes[7]; // Size of message to be received in bytes
} SBD_MT_MESSAGE;

SBD_MT_MESSAGE mtMessage;

// Structure to store device online/offline states
struct struct_online
{
  bool rtc = false;
  bool imu = false;
  bool gnss = false;
  bool iridium = false;
  bool bme280 = false;
} online;

// Union to store loop timers
struct struct_timer
{
  unsigned long rtc;
  unsigned long syncRtc;
  unsigned long sensors;
  unsigned long imu;
  unsigned long gnss;
  unsigned long iridium;
} timer;

// ------------------------------------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------------------------------------
void setup()
{
  // Pin assignments
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_IRIDIUM_EN, OUTPUT);
  pinMode(PIN_GNSS_EN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PIN_GNSS_EN, LOW);     // Disable power to GNSS
  digitalWrite(PIN_IRIDIUM_EN, LOW);  // Disable power to Iridium 9603

  // Set analog resolution to 12-bits
  analogReadResolution(12);

  // Apply ADC gain and offset error calibration correction
  analogReadCorrection(17, 2057);

  Wire.begin(); // Initialize I2C
  Wire.setClock(400000); // Set I2C clock speed to 400 kHz

#if DEBUG
  SERIAL_PORT.begin(115200); // Begin serial at 115200 baud
  //while (!SERIAL_PORT); // Wait for user to open Serial Monitor
  blinkLed(2, 1000); // Non-blocking delay to allow user to open Serial Monitor
#endif

  enableGnssPower(); // Enable power to GNSS
  configureLed(); // Configure WS2812B RGB LED

  DEBUG_PRINTLN();
  printLine();
  DEBUG_PRINTLN("Cryologger - Iceberg Tracking Beacon v3.0");
  printLine();

  // Configure devices
  configureWatchdog();    // Configure Watchdog Timer (WDT)
  configureRtc();         // Configure real-time clock (RTC)
  configureGnss();        // Configure GNSS receiver
  syncRtc();              // Synchronize RTC with GNSS
  configureImu();         // Configure interial measurement unit (IMU)
  configureSensors();     // Configure attached sensors
  configureIridium();     // Configure Iridium 9603 transceiver

  setInitialAlarm();      // Configure and set intial RTC alarm

  DEBUG_PRINT("Info: "); printCurrentDateTime();
  DEBUG_PRINT("Info: Initial alarm "); printAlarm();

  setLedColour(CRGB::White); // Change LED colour to indicate completion of setup
}

// ------------------------------------------------------------------------------------------------
// Loop
// ------------------------------------------------------------------------------------------------
void loop()
{
  // Check if alarm ISR flag was set (protects against false triggers)
  if (alarmFlag)
  {
    alarmFlag = false; // Clear alarm ISR flag
    readRtc(); // Read the RTC

    // Check if RTC alarm flag was set
    if (rtc.getInterruptFlag(FLAG_ALARM) || firstTimeFlag)
    {
      // Check if program is running for the first time
      if (!firstTimeFlag)
      {
        wakeUp();
      }

      DEBUG_PRINT("Info: Alarm interrupt trigger ");
      printDateTime(); // Print RTC date and time at time of alarm interrupt

      // Perform measurements
      petDog();         // Reset the Watchdog Timer
      readBattery();    // Read the battery voltage
      readSensors();    // Read attached sensors
      readImu();        // Read the IMU
      readGnss();       // Read the GNSS
      writeBuffer();    // Write the data to transmit buffer
      transmitData();   // Transmit data
      printTimers();    // Print function execution timers
      setRtcAlarm();    // Set the RTC alarm

      DEBUG_PRINTLN("Info: Entering deep sleep...");
      DEBUG_PRINTLN();

      // Disable serial
      disableSerial();
    }
  }

  // Check for Watchdog Timer interrupts
  if (watchdogFlag)
  {
    petDog(); // Reset the Watchdog Timer
  }

  // Blink LED to indicate WDT interrupt and nominal system operation
  blinkLed(1, 10);

  // Enter deep sleep and wait for WDT or RTC alarm interrupt
  goToSleep();
}
