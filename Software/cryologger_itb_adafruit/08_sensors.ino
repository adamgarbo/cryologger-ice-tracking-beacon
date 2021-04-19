// Configure attached sensors
void configureSensors()
{
  DEBUG_PRINT("Info: Initializing BME280...");
  // SparkFun BME280 Configuration
  if (bme280.begin())
  {
    bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                       Adafruit_BME280::SAMPLING_X1, // Temperature
                       Adafruit_BME280::SAMPLING_X1, // Pressure
                       Adafruit_BME280::SAMPLING_X1, // Humidity
                       Adafruit_BME280::FILTER_OFF);
    online.bme280 = true;
    DEBUG_PRINTLN("success!");
  }
  else
  {
    DEBUG_PRINTLN("failed!");
    online.bme280 = false;
  }
}

// Read attached sensors
void readSensors()
{
  // Start the loop timer
  unsigned long loopStartTime = millis();

  // Check if sensor(s) online
  if (online.bme280)
  {
    DEBUG_PRINT("Info: Reading BME280...");
    setLedColour(CRGB::Yellow); // Change LED colour

    // Wake sensor and return to sleep once measurement is made
    bme280.takeForcedMeasurement();

    // Delay required to allow sensor to perform the measurement
    myDelay(1000);

    // Record measurements
    float temperature = bme280.readTemperature();
    float humidity = bme280.readHumidity();
    float pressure = bme280.readPressure();

    // Write data to union
    moMessage.temperature = temperature * 100;
    moMessage.humidity = humidity * 100;
    moMessage.pressure = pressure / 10;

    DEBUG_PRINTLN("done.");
    
    //DEBUG_PRINT(F("temperature: ")); DEBUG_PRINTLN(temperature);
    //DEBUG_PRINT(F("humidity: ")); DEBUG_PRINTLN(humidity);
    //DEBUG_PRINT(F("pressure: ")); DEBUG_PRINTLN(pressure);
    
    // Stop the loop timer
    timer.sensors = millis() - loopStartTime;

    setLedColour(CRGB::Green); // Change LED colour

  }
  else
  {
    DEBUG_PRINTLN("Warning: BME280 offline!");
    setLedColour(CRGB::Red); // Change LED colour
  }
}