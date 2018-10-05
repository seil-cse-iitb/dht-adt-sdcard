/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10

 * for RTC
 * SCL - A5
 * SDA - A4

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
//#include <malloc.h>
#include <dht.h> // src: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable

// src: http://jeelabs.org/2012/05/04/measuring-vcc-via-the-bandgap/
// https://github.com/jcw/jeelib/blob/master/examples/Ports/bandgap/bandgap.ino
#include <avr/sleep.h>

//Constants
#define DHT22_PIN_1 7     // dht sensor1
#define DHT22_PIN_2 8     // dht sensor2

#define chipSelect 10 // SD card driver

//#define SLEEP_DELAY_IN_SECONDS  5  // value in secs
#define SLEEP_DELAY_IN_SECONDS  120  // value in secs

char node_id_String[] = "5";

unsigned long log_count = 0L; //sequence number of data entries //resets whenever node is restarted
char log_countString[10];

char temperatureString1[6];
char humidityString1[6];
char temperatureString2[6];
char humidityString2[6];

char vccString[6];
char result[50];

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

  if (!SD.begin(chipSelect)) {
    Serial.println(F("initialization failed!"));
    return;
  }
  Serial.println(F("initialization done."));

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //always comment this statement after setting the RTC time
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, please set the time!"));
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

void _read_dht_data(dht& _dht, int _pin)
{
  struct
  {
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
  } stat = { 0, 0, 0, 0, 0, 0, 0, 0}; //for debugging dht errors

  // READ DATA
  Serial.print("DHT22, \t");

  pinMode(_pin, INPUT_PULLUP);

  uint32_t start = micros();
  int chk = _dht.read22(_pin);
  uint32_t stop = micros();

  pinMode(_pin, INPUT); //test power consumption by commenting this line

  stat.total++;
  switch (chk)
  {
    case DHTLIB_OK:
      stat.ok++;
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      stat.crc_error++;
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      stat.time_out++;
      Serial.print("Time out error,\t");
      break;
    default:
      stat.unknown++;
      Serial.print("Unknown error,\t");
      break;
  }
  // DISPLAY DATA
  //    Serial.print(DHT.humidity, 1);
  //  Serial.print(DHT.humidity);
  //  Serial.print(",\t");
  //  //    Serial.print(DHT.temperature, 1);
  //  Serial.print(DHT.temperature);
  //  Serial.print(",\t");
  //  Serial.print(stop - start);
  Serial.println();

  if (stat.total % 20 == 0)
  {
    Serial.println("\nTOT\tOK\tCRC\tTO\tUNK");
    Serial.print(stat.total);
    Serial.print("\t");
    Serial.print(stat.ok);
    Serial.print("\t");
    Serial.print(stat.crc_error);
    Serial.print("\t");
    Serial.print(stat.time_out);
    Serial.print("\t");
    Serial.print(stat.connect);
    Serial.print("\t");
    Serial.print(stat.ack_l);
    Serial.print("\t");
    Serial.print(stat.ack_h);
    Serial.print("\t");
    Serial.print(stat.unknown);
    Serial.println("\n");
  }
}

void get_temp_humidity(dht& local_dht, int pin) {
  _read_dht_data(local_dht, pin); //discard the first reading as DHT22 returns saved
  // values from memory - refer: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  //  delay(2000); //wait for 2 secs before taking new reading // may be we can use sleep routine to save power
  //  _read_dht_data(local_dht, pin); //use this reading
}

//void get_temp_humidity(float* temp, float* hum ) { //using pointers to modifiy two values
//  read_dht_data();
//  *temp = DHT.temperature;
//  *hum = DHT.humidity;
//}

void loop()
{
  unsigned long start_time = millis(); 

  dht dht1; //obj of class dht
  dht dht2; //obj of class dht
  //  float temperature = 0.0;
  //  float humidity = 0.0;
  //  get_temp_humidity(&temperature, &humidity); //using pointers to modifiy two values

  //discard the first reading as DHT22 returns saved values from memory
  //refer: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  get_temp_humidity(dht1, DHT22_PIN_1);
  get_temp_humidity(dht2, DHT22_PIN_2);
  delay(2000); //wait for 2 secs before taking new reading // may be we can use sleep routine to save power
  get_temp_humidity(dht1, DHT22_PIN_1);
  get_temp_humidity(dht2, DHT22_PIN_2);

  get_RTC_timestamp(); //get the timestamp of the latest reading

  float temperature1 = dht1.temperature;
  float humidity1 = dht1.humidity;
  float temperature2 = dht2.temperature;
  float humidity2 = dht2.humidity;

  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature1, 2, 2, temperatureString1);
  dtostrf(humidity1, 2, 2, humidityString1);
  dtostrf(temperature2, 2, 2, temperatureString2);
  dtostrf(humidity2, 2, 2, humidityString2);
  //convert long to string using decimal format
  ltoa(log_count, log_countString, 10); // works
  //  sprintf(log_countString, "%lu", log_count); // %lu => unsigned long //works

  float myVCC = (float)readVcc(); // in millivolts
  myVCC = myVCC / 1000.0; // in Volts
  //convert float to string using decimal format
  dtostrf(myVCC, 2, 2, vccString);

  //prepare the data string to be written to file
  strcpy(result, node_id_String);
  strcat(result, ",");
  strcat(result, log_countString);
  strcat(result, ",");
  strcat(result, now_String.c_str());
  strcat(result, ",");
  strcat(result, temperatureString1);
  strcat(result, ",");
  strcat(result, humidityString1);
  strcat(result, ",");
  strcat(result, temperatureString2);
  strcat(result, ",");
  strcat(result, humidityString2);
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

  int adjusted_sleep_time = SLEEP_DELAY_IN_SECONDS - (loop_time/1000); // in secs

  if(time_truncation_error >= 1000){  //adjust the cumulative time difference in millis when error goes above 1 sec
    adjusted_sleep_time = adjusted_sleep_time - (time_truncation_error/1000);     // subtracting 1 sec 
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


