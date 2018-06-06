/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * weather.h : Header for the weather station functions
 *
 * Created: August 2005
 *
 * Authors: Robert Hubley <rhubley@gmail.com>
 *          Wendi Ruef <wruef@ocean.washington.edu>
 * 
 * See LICENSE for conditions of use.
 *
 * $Id$
 *
 *********************************************************************
 * $Log$
 *
 *********************************************************************
 *
 */
#ifndef _WEATHER_H
#define _WEATHER_H


enum downloadTypes {
  LOGLST,
  LOGALL
};


// 
// weatherDMPRevB: 
//   The standard data record returned by the DAVIS
//   weather station using the DMP command.  See 
//   manual for relevant firmware dates.  It is very
//   important that this data structure be packed
//   as we make a cast from a char[] to this structure
//   at some point in the code.  A "packed" structure
//   is one in which all fields follow each other 
//   in memory. If this is not specified the compiler
//   will attempt to place unused bytes between fields
//   so that each field begins on an even word boundry.
//   This is an optimization which breaks the cast
//   we are making.
//
struct weatherDMPRevB {
  // Date Stamp: offset 0, size 2 
  //  These 16 bits hold the date that the archive was 
  //  written in the following format:
  //    Year (7 bits) | Month (4 bits) | Day (5 bits) 
  //  or
  //    day + ( month * 32 ) + ( ( year - 2000 ) * 512 )
  unsigned short int  dateStamp;

  // Time on the Vantage that the archive record was
  // written:
  //   (Hour * 100) + minute.
  unsigned short int  timeStamp;

  // Either the average outside temperature, or the
  // final outside temperature over the archive period.
  // Unites are ( degress F / 10 )
  short int  outsideTemperature;

  // Highest outside temp over the archive period.
  short int  highOutTemperature;

  // Lowest outside temp over the archive period.
  short int  lowOutTemperature;

  // Number of rain clicks over the archive period. ( one click
  // is 1/100th of an inch ).
  unsigned short int  rainfall;

  // Highest rain rate over the archive period, or the
  // rate shown on the console at the end of the period
  // if there was no rain. Units are ( rain clicks / hour ).
  // One click is 1/100th of an inch.
  unsigned short int  highRainRate;

  // Barometer reading at the end of the archive period.
  // Unites are ( in Hg / 1000 ).
  unsigned short int  barometer;

  // Average solar radiation over the archive period.
  // Units are ( Watts / m^2 ).
  unsigned short int  solarRadiation;

  // Number of packets containing wind speed data
  // received from the ISS or wireless anemometer.
  unsigned short int  numWindSamples;

  // Either the average inside temperature, or the final 
  // inside temperature over the archive period. Units
  // are ( deg F / 10 ).
  short int  insideTemperature;

  // Inside humidity at the end of the archive period.
  unsigned char insideHumidity;

  // Outside humidity at the end of the archive period.
  unsigned char outsideHumidity;

  // Average wind speed over the archive interval.
  // Units are ( MPH ).
  unsigned char averageWindSpeed;

  // Highest wind speed over the archive interval.
  // Units are ( MPH ).
  unsigned char highWindSpeed;

  // Direction code of the high wind speed.
  // 0 = N, 1 = NNE, 2 = NE, ... 14 = NW, 15 = NNW,
  // 255 = Error! 
  unsigned char directionOfHighWindSpeed;

  // Prevailing or dominant wind direction code.
  // 0 = N, 1 = NNE, 2 = NE, ... 14 = NW, 15 = NNW,
  // 255 = Error! 
  unsigned char prevailingWindDirection;

  // Average UV index.  Units are ( UV Index / 10 ).
  unsigned char averageUVIndex;

  // ET accumulated over the last hour. Only records
  // "on the hour" will have a non-zero value. Units
  // are ( in / 1000 ).
  unsigned char ET;

  // Highest solar radiation value over the archive
  // period. Units are ( Watts / m^2 ).
  unsigned short int  highSolarRadiation;

  // Highest UV index value over the archive period.
  // Units are ( Watts / m^2 ).
  unsigned char highUVIndex;

  // Weather forecast rule at the end of the archive
  // period.
  unsigned char forecastRule;

  // 2 Leaf temperature values. Units are ( deg F + 90 ).
  char leafTemperature1;
  char leafTemperature2;

  // 2 Leaf wetness values. Range is 0 - 15
  unsigned char leafWetness1;
  unsigned char leafWetness2;

  // 4 Soil temperature values. Units are ( deg F + 90 ).
  unsigned char soilTemperature1;
  unsigned char soilTemperature2;
  unsigned char soilTemperature3;
  unsigned char soilTemperature4;

  // 0xFF = Rev A, 0x00 = Rev B archive record.
  unsigned char downloadRecordType;

  // 2 Extra humidity values.
  unsigned char extraHumidity1;
  unsigned char extraHumidity2;

  // 3 Extra temperature values. Units are ( deg F + 90 ).
  unsigned char extraTemperature1;
  unsigned char extraTemperature2;
  unsigned char extraTemperature3;

  // 4 Soil moisture values. Units are ( cb ).
  unsigned char soilMisture1;
  unsigned char soilMisture2;
  unsigned char soilMisture3;
  unsigned char soilMisture4;

}__attribute__ ((packed)); 






struct weatherLOOPRevB {

  // Spells out "LOO" for Rev B packets
  char LOOConst[3];

  // Signed byte that indicates the current 
  // 3-hour barometer trend. It is one of these
  // values:
  //   -60 = Falling Rapidly = 196 ( as an unsigned byte )
  //   -20 = Falling Slowly  = 236 
  //     0 = Steady
  //    20 = Rising Slowly 
  //    60 = Rising Rapidly
  //    80 = No tren info is available.
  //    Any other value means that the Vantage does
  //    not have the 3 hours of data needed to determine
  //    the bar trend.
  char barTrend;

  // Has the value zero ( Currently unused )
  char packetType;
  
  // Location in the archive memory where the next
  // data packet will be written.  This can be 
  // monitored to detect when a new record is
  // created.
  unsigned short int nextRecord;

  // Current barometer.  Units are ( in Hg / 1000 ). The
  // barometer value should be between 20 inches and 32.5.
  // Values outside this range will not be logged.
  unsigned short int barometer;

  // The value is sent as 10th of a degree in F.  For
  // example, 795 is returned for 79.5F.
  short int insideTemperature;

  // This is the relative humidity in %, such as 50
  // is returned for 50%
  unsigned char insideHumidity;

  // The value is sent as 10th of a degree in F.  For
  // example, 795 is returned for 79.5F.
  short int outsideTemperature;

  // It is a byte unsigned value in mph.  If there is
  // a problem reading the wind speed this will be forced
  // to 0.
  unsigned char windSpeed;

  // It is a usigned byte in mph.
  unsigned char windSpeed10MinAvg;

  // It is a two byte unsigned value from 0 to 360 degrees
  // ( 0 degrees is North, 90 degrees is East, 180 degrees
  //   is South and 270 degrees is West ).
  unsigned short int windDirection;

  // These fields support seven extra temperature stations.
  // Each byte is one extra temperature value in whole
  // degrees F with an offset of 90 degrees.  For example
  // a value of 0 = -90 F; and a value of 169 = 79 F
  unsigned char extraTemp1;
  unsigned char extraTemp2;
  unsigned char extraTemp3;
  unsigned char extraTemp4;
  unsigned char extraTemp5;
  unsigned char extraTemp6;
  unsigned char extraTemp7;
  
  // These fields support four soil temperature sensors, 
  // in the same format as the Extr Temperature fields above.
  unsigned char extraSoilTemp1;
  unsigned char extraSoilTemp2;
  unsigned char extraSoilTemp3;
  unsigned char extraSoilTemp4;

  // These fields support four leaf temperature sensors, 
  // in the same format as the Extr Temperature fields above.
  unsigned char extraLeafTemp1;
  unsigned char extraLeafTemp2;
  unsigned char extraLeafTemp3;
  unsigned char extraLeafTemp4;

  // This is the relative humidity in %
  unsigned char outsideHumidity;

  // Relative humidity in % for extra seven humidity stations.
  unsigned char extraHumidities1;
  unsigned char extraHumidities2;
  unsigned char extraHumidities3;
  unsigned char extraHumidities4;
  unsigned char extraHumidities5;
  unsigned char extraHumidities6;
  unsigned char extraHumidities7;

  // This value is sent as 100th of an inch per hour.
  // For example, 256 represents 2.56 inches/hour.
  unsigned short int rainRate;

  // The unit is UV Index.
  unsigned char uv;
  
  // The unit is in watt/meter^2
  unsigned short int solarRadiation;

  // The storm is stored as 100th of an inch.
  unsigned short int stormRain;

  // Bit 15 to bit 12 is the month, bit 11 to bit 7 
  // is the day and bit 6 to bit 0 is the year offset
  // by 2000.
  unsigned short int startDateOfCurrentStorm;

  // 100th of an inch.
  unsigned short int dayRain;

  // 100th of an inch.
  unsigned short int monthRain;

  // 100th of an inch.
  unsigned short int yearRain;

  // 100th of an inch.
  unsigned short int dayET;

  // 100th of an inch.
  unsigned short int monthET;

  // 100th of an inch.
  unsigned short int yearET;

  // The unit is in centibar. It supports four
  // soil sensors.
  unsigned char soilMoisture1;
  unsigned char soilMoisture2;
  unsigned char soilMoisture3;
  unsigned char soilMoisture4;

  // This is a number between 0-15 with 0 = very dry
  // and 15 = very wet.  It support four sensors.
  unsigned char leafWetness1;
  unsigned char leafWetness2;
  unsigned char leafWetness3;
  unsigned char leafWetness4;

  // Currently active alarms. See table in manual.
  unsigned char insideAlarms;

  // Currently active alarms. See table in manual.
  unsigned char rainAlarms;

  // Currently active alarms. See table in manual.
  unsigned short int outsideAlarms;

  // Currently active alarms. See table in manual.
  unsigned char extraTempHumidAlarms1;
  unsigned char extraTempHumidAlarms2;
  unsigned char extraTempHumidAlarms3;
  unsigned char extraTempHumidAlarms4;
  unsigned char extraTempHumidAlarms5;
  unsigned char extraTempHumidAlarms6;
  unsigned char extraTempHumidAlarms7;
  unsigned char extraTempHumidAlarms8;

  // Currently active alarms. See table in manual.
  unsigned char soilLeafAlarms1;
  unsigned char soilLeafAlarms2;
  unsigned char soilLeafAlarms3;
  unsigned char soilLeafAlarms4;

  // ??
  unsigned char txmtBatteryStatus;

  // Voltage = ((Data * 300)/512)/100.0
  unsigned short int consoleBatteryVoltage;

  // ??
  unsigned char forecastIcons;

  // ??
  unsigned char forecastRuleNumber;

  // The time is stored as hour * 100 + min.
  unsigned short int timeOfSunrise;

  // The time is stored as hour * 100 + min.
  unsigned short int timeOfSunset;

  // ??
  unsigned char lineFeedConst;
  unsigned char carriageReturnConst;

  // CRC
  unsigned short int crc; 

}__attribute__ ((packed)); 



int syncWSTime( int fd );
int setWSTime( int fd, struct tm *time );
struct tm *getWSTime( int fd );
int downloadWeatherData( int wsFD, FILE * outFile, int lastRecordOnly );
int logInstantWeather ( int wsFD, FILE * outFile );
int logDMPRecord( FILE * outFile , struct weatherDMPRevB *rec );
int initializeWeatherStation( int wsFD );


#endif
