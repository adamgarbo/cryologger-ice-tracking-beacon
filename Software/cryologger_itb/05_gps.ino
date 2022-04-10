// Read GPS
void readGps()
{
  // Start loop timer
  unsigned long loopStartTime = millis();

  // Clear flags
  bool fixFound = false;
  bool charsSeen = false;

  // Reset GPS fix counter
  byte fixCounter = 0;

  // Enable power to GPS
  enableGpsPower();

  DEBUG_PRINTLN("Info: Beginning to listen for GPS traffic...");

  GPS_PORT.begin(9600);
  myDelay(1000);

  // Configure GPS
  GPS_PORT.println("$PMTK220,1000*1F"); // Set NMEA update rate to 1 Hz
  myDelay(100);
  GPS_PORT.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // Set NMEA sentence output frequencies to GGA and RMC
  myDelay(100);
  //GPS_PORT.println("$PGCMD,33,1*6C"); // Enable antenna updates
  //GPS_PORT.println("$PGCMD,33,0*6D"); // Disable antenna updates

  // Look for GPS signal for up to gpsTimeout
  while (!fixFound && millis() - loopStartTime < gpsTimeout * 60UL * 1000UL)
  {
    if (GPS_PORT.available())
    {
      charsSeen = true;
      char c = GPS_PORT.read();
#if DEBUG_GPS
      Serial.write(c); // Echo NMEA sentences to serial
#endif
      if (gps.encode(c))
      {
        if ((gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) &&
            (gps.location.isUpdated() && gps.date.isUpdated() && gps.time.isUpdated()) &&
            (gps.satellites.value() > 0))
        {

          fixCounter++; // Increment fix counter

          // Wait until a specified number of GPS fixes have been collected
          if ((fixCounter >= 50) && (gps.satellites.value() > 0))
          {
            fixFound = true;

            // Convert GPS date and time to epoch time
            tm.Hour = gps.time.hour();
            tm.Minute = gps.time.minute();
            tm.Second = gps.time.second();
            tm.Day = gps.date.day();
            tm.Month = gps.date.month();
            tm.Year = gps.date.year() - 1970; // Offset from 1970
            unsigned long gpsEpoch = makeTime(tm); // Change the tm structure into time_t (seconds since epoch)

            // Get RTC epoch time
            unsigned long rtcEpoch = rtc.getEpoch();

            // Calculate RTC drift
            long rtcDrift = rtcEpoch - gpsEpoch;

            DEBUG_PRINTLN("");
            DEBUG_PRINT(F("gpsEpoch: ")); DEBUG_PRINTLN(gpsEpoch);
            DEBUG_PRINT(F("rtcEpoch: ")); DEBUG_PRINTLN(rtcEpoch);
            DEBUG_PRINT(F("unixtime: ")); DEBUG_PRINTLN(unixtime);

            // Sync RTC with GPS date and time only if gpsEpoch is in the future
            if (((gpsEpoch > 1630368000) && (gpsEpoch > unixtime)) || firstTimeFlag)
            {
              rtc.setEpoch(gpsEpoch);
              DEBUG_PRINT(F("Info: RTC synced ")); printDateTime();
            }
            else
            {
              DEBUG_PRINT(F("Warning: RTC not synced! GPS time in the past! ")); printDateTime();
            }

            // Write data to buffer
            moSbdMessage.latitude = gps.location.lat() * 1000000;
            moSbdMessage.longitude = gps.location.lng() * 1000000;
            moSbdMessage.satellites = gps.satellites.value();
            moSbdMessage.hdop = gps.hdop.value();

            DEBUG_PRINT(F("Info: RTC drift ")); DEBUG_PRINT(rtcDrift); DEBUG_PRINTLN(F(" seconds"));
            blinkLed(5, 250);
          }
        }
      }
    }

    // Call callback during acquisition of GPS fix
    ISBDCallback();

    // Exit function if no GPS data is received,
    if ((millis() - loopStartTime) > 5000 && gps.charsProcessed() < 10)
    {
      DEBUG_PRINTLN(F("Warning: No GPS data received. Please check wiring."));
      break;
    }
  }

  if (!fixFound)
  {
    DEBUG_PRINTLN(F("Warning: No GPS fix found!"));
  }

  // Close GPS port
  GPS_PORT.end();

  // Disable power to GPS
  disableGpsPower();

  // Stop the loop timer
  timer.gps = millis() - loopStartTime;
}
