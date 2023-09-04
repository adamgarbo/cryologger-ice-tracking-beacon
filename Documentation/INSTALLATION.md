# Cryologger Ice Tracking Beacon (ITB) - Installation
This guide provides step-by-step instructions on installing the Arduino IDE and required board definitions and libraries, which are necessary for uploading code to the Cryologger ITB.

## Step 1: Download Arduino IDE  
* Navigate to https://www.arduino.cc/en/software and download the Legacy 1.8.x version of the Arduino IDE
![image](/Images/arduino-ide-download.png)

## Step 2: Add Additional Boards Manager URL:
* In the Arduino IDE navigate to: Preferences
* Add the following "Additional Boards Manager URL" as shown in the screenshot below:  
```https://adafruit.github.io/arduino-board-index/package_adafruit_index.json```
* Also during this step, check the "compile" and "upload" boxes for "Show verbose output during" and change "Compiler warnings" to "Default"
![image](/Images/arduino-ide-preferences.png)

## Step 3: Install Board Definitions
* Navigate to: Tools > Boards > Boards Manager
* Search for: SAMD Boards
* Select and install version: Arduino SAMD Boards v1.8.13 
* Select and install version: Adafruit SAMD Boards v1.7.13
![image](/Images/arduino-ide-boards.png)

## Step 4: Install Libraries
Several libraries are required by the Cryologger ITB. These can be downloaded using the Arduino IDE's Library Manager (requires an Internet connection).

* Navigate to: Tools > Manage Libraries
* Search for and install the following libraries:

**Table 1.** Libraries required by Cryologger ITB. Last updated 2023-06-19.
| Library                 | Version | GitHub Repository                                                   |
|-------------------------|---------|---------------------------------------------------------------------|
| Adafruit BME280         | 2.2.2   | https://github.com/adafruit/Adafruit_BME280_Library                 |
| Adafruit LSM303 Accel   | 1.1.6   | https://github.com/adafruit/Adafruit_LSM303_Accel                   |
| Adafruit LIS2MDL        | 2.1.4   | https://github.com/adafruit/Adafruit_LIS2MDL                        |
| Adafruit Unified Sensor | 1.1.9   | https://github.com/adafruit/Adafruit_Sensor                         |
| Arduino Low Power       | 1.2.2   | https://github.com/arduino-libraries/ArduinoLowPower                |
| IridiumSBD              | 3.0.6   | https://github.com/sparkfun/SparkFun_IridiumSBD_I2C_Arduino_Library |
| RTCZero                 | 1.6.0   | https://github.com/arduino-libraries/RTCZero                        |
| TimeLib                 | 1.6.1   | https://github.com/PaulStoffregen/Time                              |
| TinyGPSPlus             | 1.0.3   | https://github.com/mikalhart/TinyGPSPlus                            |

![image](/Images/arduino-ide-library.png)

## Step 5: Download Software
Code for the Cryologger ITB is made available on the following GitHub repository:
* [https://github.com/adamgarbo/Cryologger Ice_Tracking_Beacon](https://github.com/adamgarbo/Cryologger_Ice_Tracking_Beacon)
* Click on "Releases" and download the .zip file of the most recent release:


## Step 6: Test Program Compilation
* Navigate to the /Software/Arduino/cryologger_itb folder of the downloaded repository
* Double-click on the `cryologger_aws.ino` file
* Click on the checkmark in the upper left corner of the Arduino IDE program window
* Watch debugging window for compilation errors

![image](/Images/arduino-ide-verify.png)

## Step 7: Connect Hardware & Configure Port Settings
* Connect to the Cryologger ITB using a micro USB cable.
* In the Arduino IDE click on Tools > Board > Adafruit SAMD > Adafruit Feather M0 (SAMD)

![image](/Images/arduino-ide-board-1.png)
![image](/Images/arduino-ide-board-2.png)
![image](/Images/arduino-ide-board-3.png)

* Next, click on Tools > Port and select the appropriate serial port for the Adafruit Feather M0 Adalogger
![image](/Images/arduino-ide-port-1.png)
![image](/Images/arduino-ide-port-2.png)

## Step 8: Upload Program
* Once the code has compiled successfully, click on the right-pointed arrow to upload the code
* Watch the output window for compilation errors and/or success messages
* If no errors are presented, the code has now been successfully uploaded! 
![image](/Images/arduino-ide-upload.png)

## Step 9: Observe Serial Monitor
* After successfully uploading the program, click on the magnifying glass in the top right-hand corner to open the Serial Monitor
* Click on the baud dropdown and select 115200 baud
* Ensure the data logging mode and settings are correct
![image](/Images/arduino-ide-serial-monitor-1.png)
![image](/Images/arduino-ide-serial-monitor-2.png)

## Step 10: Next Steps
* The detailed operation of the Cryologger ITB will be covered next in OPERATION.md
