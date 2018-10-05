/*
 * for UNO
 * SCL = A5
 * SDA = A4
 *
 */

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include "Time.h" // https://github.com/PaulStoffregen/Time 
//#include "ArduinoSTL.h"
//#include "ctime"

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//struct tm;

//String dataString = "";

void setup () {

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  Serial.begin(9600);

  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //always comment this statement after setting the RTC time
  if (rtc.lostPower()) 
  {
    Serial.println("RTC lost power, please set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //always comment this statement after setting the RTC time
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop () {
  DateTime dt_now = rtc.now();

  //    Serial.print(dt_now.year(), DEC);
  //    Serial.print('/');
  //    Serial.print(dt_now.month(), DEC);
  //    Serial.print('/');
  //    Serial.print(dt_now.day(), DEC);
  //    Serial.print(" (");
  //    Serial.print(daysOfTheWeek[dt_now.dayOfTheWeek()]);
  //    Serial.print(") ");
  //    Serial.print(dt_now.hour(), DEC);
  //    Serial.print(':');
  //    Serial.print(dt_now.minute(), DEC);
  //    Serial.print(':');
  //    Serial.print(dt_now.second(), DEC);
  //    Serial.println();

  //    Serial.print(" since midnight 1/1/1970 = ");

  time_t ts_now = dt_now.unixtime();
  Serial.print("epochtime without timezone = ");
  Serial.println(ts_now);
  Serial.print("epochtime in IST = ");
  Serial.println(ts_now - 19800);

  //    Serial.print(dt_now.unixtime() / 86400L);
  //    Serial.println("d");


  // Assemble Strings to log data
  String theyear = String(dt_now.year(), DEC);
  String themon = String(dt_now.month(), DEC);
  String theday = String(dt_now.day(), DEC);
  String thehour = String(dt_now.hour(), DEC);
  String themin = String(dt_now.minute(), DEC);
  String thesec = String(dt_now.second(), DEC);

  //Put all the time and date strings into one String, strftime('%Y/%m/%d %H:%M:%S')
  String now_String = String(theyear + "/" + themon + "/" + theday + " " + thehour + ":" + themin + ":" + thesec);
  Serial.println(now_String);


  Serial.println("*****************");
  Serial.println();

  delay(5000);
}
