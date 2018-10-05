/*
  SD card read/write

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10

   for RTC
   SCL - A5
   SDA - A4

  This example code is in the public domain.

*/

#include "LowPower.h"
#include <SPI.h>
#include <SD.h>

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include "Time.h" // https://github.com/PaulStoffregen/Time 
//#include "ArduinoSTL.h"
//#include "ctime"

#include <string.h>
#include <avr/sleep.h>

//#define chipSelect 7 // SD card driver

#define ADT7420Address 0x48
#define ADT7420TempReg 0x00
#define ADT7420ConfigReg 0x03

int chipSelect = 7;

#define SLEEP_DELAY_IN_SECONDS  600  // value in secs
//#define SLEEP_DELAY_IN_SECONDS  120  // value in secs

char node_id_String[] = "16";

unsigned long log_count = 0L; //sequence number of data entries //resets whenever node is restarted
char log_countString[10];

struct data_to_be_send {
  byte node_id = 1;
  float temperatureData = 0.0;
  float humidityData = 0.0;
  float battery_voltage = 0.0;
  //int co2Data = 0;
  //int tvocData = 0;
} data;

//char temperatureString1[6];
//char humidityString1[6];
//char temperatureString2[6];
//char humidityString2[6];

char tempString[6];
char humidityString[6];
char vccString[5];
char result[32];;

File myFile;
char myFileName[11] = "data"; //max length of filename; keep filename short, node id will be appended to filename
//char myFileName[11] = "SEIL";

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//struct tm;

//String dataString = "";
String now_String;
unsigned long time_truncation_error = 0; // to keep track of millisecs less than 1 secs


void setup()
{

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //  delay(3000); // wait for console opening
  init_global(); //initialise any important variables

  Serial.print(F("Initializing SD card..."));

  Wire.begin();
  if (!SD.begin(chipSelect)) {
    Serial.println(F("initialization failed!")); strcat(result, log_countString);
    strcat(result, ",");
    strcat(result, now_String.c_str());
    strcat(result, ",");
    return;
  }
  Serial.println(F("initialization done."));

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //always comment this statement after setting the RTC time
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, please set the time!")); strcat(result, log_countString);
    strcat(result, ",");
    strcat(result, now_String.c_str());
    strcat(result, ",");
    // following line sets the RTC to the date & time this sketch was compiled
    //    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //always comment this statement after setting the RTC time
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //  sd_read();
  //  delay(5000);

}

void init_global() {
  Serial.println(F("Initializing few variables..."));
  //  strcpy(myFileName, "data");
  strcat(myFileName, node_id_String); //append node-id to filename
  strcat(myFileName, ".csv");
  Serial.print(F("Filename is: "));
  Serial.println(myFileName);
  Serial.print(F("Sleep delay in secs is: "));
  Serial.println(SLEEP_DELAY_IN_SECONDS);
}


///*
long readVcc() {

  float internalREF = 1.1; // ideal
  //  float calibration_factor = 1.29 * 0.82;// calibration_factor = Vcc1 (given by voltmeter) / Vcc2 (given by readVcc() function)
  float calibration_factor = 1.19;
  internalREF = internalREF * calibration_factor;
  long scale_constant = internalREF * 1023 * 1000;

  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(20); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  long myVcc = scale_constant / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return myVcc; // Vcc in millivolts
}
//*/

void get_RTC_timestamp() {
  DateTime dt_now = rtc.now();

  time_t ts_now = dt_now.unixtime();
  //  Serial.print("epochtime without timezone = ");
  //  Serial.println(ts_now);
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
  now_String = String(theyear + "/" + themon + "/" + theday + " " + thehour + ":" + themin + ":" + thesec);
  //  Serial.println(now_String);
  //  Serial.println("*****************");
  //  Serial.println();

  //  delay(5000);

}

void sd_write() {

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(myFileName, FILE_WRITE); //default is append mode

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to file...");
    //    myFile.println("testing 1, 2, 3.");
    myFile.println(result);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}

void sd_read() {

  // re-open the file for reading:
  myFile = SD.open(myFileName);
  if (myFile) {
    Serial.println(myFileName);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}


float readADT7420()
{
  float temp = 0;
  long tempReading = 0;

  Wire.beginTransmission(ADT7420Address);
  Wire.write(0x03);
  Wire.write(B10100000); //Set 16bit mode and one-shot mode
  Wire.endTransmission();
  //delay(20); //wait for sensor

  byte MSB;
  byte LSB;
  // Send request for temperature register.
  Wire.beginTransmission(ADT7420Address);
  Wire.write(ADT7420TempReg);
  Wire.endTransmission();
  // Listen for and acquire 16-bit register address.
  Wire.requestFrom(ADT7420Address, 2);
  MSB = Wire.read();
  LSB = Wire.read();
  // Assign global 'tempReading' the 16-bit signed value.
  tempReading = ((MSB << 8) | LSB);
  if (tempReading > 32768)
  {
    tempReading = tempReading - 65535;
    temp = (tempReading / 128.0) * -1;
  }
  else
  {
    temp = (tempReading / 128.0);
  }

  return temp;
}


void loop()
{
  unsigned long start_time = millis();

  data.temperatureData = readADT7420();
  data.humidityData = 0.0;


  data.battery_voltage = readVcc();

  get_RTC_timestamp(); //get the timestamp of the latest reading


  dtostrf(data.temperatureData, 6, 3, tempString);
  dtostrf(data.humidityData, 5, 2, humidityString);
  dtostrf(data.battery_voltage, 4, 2, vccString);

  //convert long to string using decimal format
  ltoa(log_count, log_countString, 10); // works
  //  sprintf(log_countString, "%lu", log_count); // %lu => unsigned long //works

  float myVCC = (float)readVcc(); // in millivolts
  myVCC = myVCC / 1000.0; // in Volts
  //convert float to string using decimal format
  dtostrf(myVCC, 2, 2, vccString);


  strcpy(result, node_id_String);
  strcat(result, ",");
  strcat(result, log_countString);
  strcat(result, ",");
  strcat(result, now_String.c_str());
  strcat(result, ",")
  ;  strcat(result, tempString);
  strcat(result, ",");
  strcat(result, humidityString);
  strcat(result, ",");
  strcat(result, vccString);

  Serial.println(result);


  sd_write(); //write to file
  log_count += 1; //increment the sequence number of data entry by 1
  delay(100);

  unsigned long end_time = millis(); // in msecs
  unsigned long  loop_time = (end_time - start_time); // in msecs

  Serial.print(F("loop time: "));
  Serial.println(loop_time);

  time_truncation_error = time_truncation_error + (loop_time % 1000);  //used modulo
  Serial.print(F("time_truncation_error in milli-secs: "));
  Serial.println(time_truncation_error);

  int adjusted_sleep_time = SLEEP_DELAY_IN_SECONDS - (loop_time / 1000); // in secs

  if (time_truncation_error >= 1000) { //adjust the cumulative time difference in millis when error goes above 1 sec
    adjusted_sleep_time = adjusted_sleep_time - (time_truncation_error / 1000);   // subtracting 1 sec
    time_truncation_error = (time_truncation_error % 1000);
  }

  Serial.print(F("Adjusted sleep time in secs: "));
  Serial.println(adjusted_sleep_time);

  Serial.print(F("Entering sleep mode for secs: "));
  Serial.println(adjusted_sleep_time);
  //Serial.flush();
  delay(1000);  // additional delay for serial printing -- to be adjusted during sleep, keeping delay large as 1 sec so that calc of #sleep secs is an integer
  adjusted_sleep_time = adjusted_sleep_time - 1;
  //power down
  for (int i = 0; i < adjusted_sleep_time; i++) {
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  }
  //  delay(SLEEP_DELAY_IN_SECONDS * 1000); //delay accepts value in msec
  Serial.println(F("Out of sleep mode..."));

}


