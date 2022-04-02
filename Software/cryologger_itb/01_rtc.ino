// Configure the real-time clock
void configureRtc()
{
  // Alarm modes:
  // 0: MATCH_OFF          Never
  // 1: MATCH_SS           Every Minute
  // 2: MATCH_MMSS         Every Hour
  // 3: MATCH_HHMMSS       Every Day
  // 4: MATCH_DHHMMSS      Every Month
  // 5: MATCH_MMDDHHMMSS   Every Year
  // 6: MATCH_YYMMDDHHMMSS Once, on a specific date and a specific time

  // Initialize RTC
  rtc.begin();

  // Set time manually
  //rtc.setTime(hours, minutes, seconds);
  //rtc.setDate(day, month, year);
  //rtc.setEpoch();

  // Set initial RTC alarm time
  rtc.setAlarmTime(0, 0, 0); // hours, minutes, seconds

  // Enable alarm for hour rollover match
  rtc.enableAlarm(rtc.MATCH_MMSS);

  // Attach alarm interrupt service routine (ISR)
  rtc.attachInterrupt(alarmIsr);

  alarmFlag = false; // Clear flag

  DEBUG_PRINT("Info: RTC initialized "); printDateTime();
  DEBUG_PRINT("Info: Initial alarm "); printAlarm();
  DEBUG_PRINT("Info: Alarm match "); DEBUG_PRINTLN(rtc.MATCH_MMSS);
}

// Read RTC
void readRtc()
{
  uint32_t loopStartTime = millis();

  // Get Unix Epoch time
  unixtime = rtc.getEpoch();

  // Write data to union
  moSbdMessage.unixtime = unixtime;

  // Stop loop timer
  timer.rtc = millis() - loopStartTime;
}

// Set RTC alarm
void setRtcAlarm()
{
  // Set RTC alarm according to number of consecutive failed transmissions
  if (failureCounter <= 8)
  {
    // Calculate next alarm
    alarmTime = unixtime + alarmInterval;
    DEBUG_PRINT(F("Info: unixtime ")); DEBUG_PRINTLN(unixtime);
    DEBUG_PRINT(F("Info: alarmTime ")); DEBUG_PRINTLN(alarmTime);
  }
  // Increase intervals if more than 24 hours of transmission failures
  else if (failureCounter > 8 && failureCounter <= 16)
  {
    DEBUG_PRINTLN(F("Warning: Increasing sampling interval to 3 hours and transmission interval to 9 hours!"));
    alarmTime = unixtime + 10800; // Increase sampling interval to 3 hours
  }
  // Increase intervals if more than 3 days of transmission failures
  else if (failureCounter > 16 && failureCounter <= 32)
  {
    DEBUG_PRINTLN(F("Warning: Increasing sampling interval to 6 hours and transmission interval to 36 hours!"));
    alarmTime = unixtime + 43200; // Increase sampling interval to 12 hours
  }
  // Increase intervals if more than 8 days of transmission failures
  else if (failureCounter > 32)
  {
    DEBUG_PRINTLN(F("Warning: Increasing sampling interval to 24 hours and transmission interval to 3 days!"));
    alarmTime = unixtime + 86400; // Increase sampling interval to 24 hours (1 day)
  }

  // Check if alarm was set in the past
  if ((rtc.getEpoch() >= alarmTime) || firstTimeFlag)
  {
    DEBUG_PRINTLN(F("Warning: RTC alarm set in the past or program running for the first time."));

    // Set alarm for hour rollover match
    rtc.setAlarmTime(0, 0, 0); // hours, minutes, seconds

    // Enable alarm for hour rollover match
    rtc.enableAlarm(rtc.MATCH_MMSS);

    DEBUG_PRINT("Info: "); printDateTime();
    DEBUG_PRINT("Info: Next alarm "); printAlarm();
    DEBUG_PRINT("Info: Alarm match "); DEBUG_PRINTLN(rtc.MATCH_MMSS);
  }
  // Check if alarm is set too far into the future
  else if ((alarmTime - unixtime) > 86400)
  {
    DEBUG_PRINTLN(F("Warning: RTC alarm set too far in the future!"));

    // Set alarm for hour rollover match
    rtc.setAlarmTime(0, 0, 0); // hours, minutes, seconds

    // Enable alarm for hour rollover match
    rtc.enableAlarm(rtc.MATCH_MMSS);

    DEBUG_PRINT("Info: "); printDateTime();
    DEBUG_PRINT("Info: Next alarm "); printAlarm();
    DEBUG_PRINT("Info: Alarm match "); DEBUG_PRINTLN(rtc.MATCH_MMSS);
  }
  else
  {
    DEBUG_PRINTLN(F("Info: Setting RTC alarm based on specified interval."));

    // Set alarm time
    rtc.setAlarmTime(hour(alarmTime), minute(alarmTime), 0); // hours, minutes, seconds

    // Set alarm date
    rtc.setAlarmDate(day(alarmTime), month(alarmTime), year(alarmTime) - 2000);

    // Enable alarm
    rtc.enableAlarm(rtc.MATCH_DHHMMSS);

    DEBUG_PRINT("Info: "); printDateTime();
    DEBUG_PRINT("Info: Next alarm "); printAlarm();
    DEBUG_PRINT("Info: Alarm match "); DEBUG_PRINTLN(rtc.MATCH_DHHMMSS);
  }
  // Clear flag
  alarmFlag = false;
}

// RTC alarm interrupt service routine (ISR)
void alarmIsr()
{
  alarmFlag = true;
}

// Print the RTC's current date and time
void printDateTime()
{
  char dateTimeBuffer[25];
  sprintf(dateTimeBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.getYear(), rtc.getMonth(), rtc.getDay(),
          rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  DEBUG_PRINTLN(dateTimeBuffer);
}

// Print the RTC's alarm
void printAlarm()
{
  char alarmBuffer[25];
  sprintf(alarmBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.getAlarmYear(), rtc.getAlarmMonth(), rtc.getAlarmDay(),
          rtc.getAlarmHours(), rtc.getAlarmMinutes(), rtc.getAlarmSeconds());
  DEBUG_PRINTLN(alarmBuffer);
}
