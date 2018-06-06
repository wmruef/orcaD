/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * weather.c : Davis Weather Station utility routines
 *             Written for the Vantage Pro2 Plus weather station
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include "general.h"
#include "orcad.h"
#include "crc.h"
#include "term.h"
#include "serial.h"
#include "log.h" 
#include "weather.h"


// This is a string version of ACK...for use in
// the serial routines.
static char ackStr[] = { 0x06, 0x00 };


//
// NAME
//   initializeWeatherStation - Initialize the weather station hardware
//
// SYNOPSIS
//   #include "weather.h"
//
//   int initializeWeatherStation( int wsFD );
//
// DESCRIPTION
//   The Davis weather station contains an onboard data logger
//   which is responsible for communicating with the various
//   optional weather intstruments.  This routine tests that
//   we can communicate with the weather station, sync
//   the weather station data loggers clock with ours,
//   clear the volatile data archive and lastly start the
//   logging of data.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int initializeWeatherStation ( int wsFD )
{
  char perBUFF[ 50 ];

  // Say hello
  LOGPRINT( LVL_DEBG, "initializeWeatherStation(): Called" );

  // Wake up the weather station
  term_flush( wsFD );
  if ( serialChat( wsFD, "\n", "\n", 1000L, "\n" ) < 1 ) 
    if ( serialChat( wsFD, "\n", "\n", 1000L, "\n" ) < 1 ) 
      if ( serialChat( wsFD, "\n", "\n", 1000L, "\n" ) < 1 ) 
        if ( serialChat( wsFD, "\n", "\n", 1000L, "\n" ) < 1 ) 
        {
          LOGPRINT( LVL_CRIT, "initializeWeatherStation(): Could not wake "
                    "up the weather station!" );
          return( FAILURE );
        }

  // Sync the bitsyx and weather station clocks
  //   The weather station doesn't have a battery backed up clock.
  //   Every time we power it on we have to set the time.
  if ( syncWSTime( wsFD ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "initializeWeatherStation(): Could not sync bitsyx and "
              "weather station clocks!" );
    return( FAILURE );
  }
  sleep( 1 );

  //
  // NOTE: Davis manual erroneously says this command returns
  //       an ACK.  Intead as you see below it returns an "OK"
  //
  snprintf( perBUFF, 50, "SETPER %d\n", 10 );
  if ( serialChat( wsFD, perBUFF, "OK", 1000L,  "OK\n\r" ) < 1 )
  {
    LOGPRINT( LVL_ALRT, "initializeWeatherStation(): Failed to set the "
              "weather station archive period." );
    return( FAILURE );
  }
  sleep( 1 );

  //
  // Clear the archive...although this may be redundant for several
  // reasons.  The first being that SETPER is supposed to clear the
  // archive.  The other is that this routine starts under the 
  // assumption that the logger has just been powered on and thus
  // the archive should already be clear.  But...lets just be safe.
  //
  if ( serialChat( wsFD, "CLRLOG\r", ackStr, 1000L, ackStr ) < 1 )
  {
    LOGPRINT( LVL_ALRT, "initializeWeatherStation(): Failed to clear "
              "weather station archive." );
    return( FAILURE );
  }
  sleep( 1 );

  //
  // While the manual says this isn't neccessary I have
  // observed that sometimes it is. :-)
  //
  if ( serialChat( wsFD, "START\n", "OK", 2000L, "OK\n\r" ) < 1 )
  {
    LOGPRINT( LVL_ALRT, "initializeWeatherStation(): Failed to start "
              "weather station logging." );
    return( FAILURE );
  }

  return( SUCCESS );
}



//
// NAME
//   logInstantWeather - routine to log instant weather data to a file desc.
//
// SYNOPSIS
//   #include "weather.h"
//
//   int logInstantWeather( int wsFD, FILE * outFile );
//
// DESCRIPTION
//   This function is used by orcad to display instantaneous
//   weather data to the screen or file.  The weather station
//   communications file descriptor should be opened and
//   ready for reading/writing.  The outFile should be opened
//   and ready for writing.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int logInstantWeather ( int wsFD, FILE * outFile ) 
{
  char buffer[128];
  struct weatherLOOPRevB *rec;
  int bytesRead;
  time_t nowTimeT;
  struct tm *nowTM;
  char nowStr[80];

  
  // Say hello
  LOGPRINT( LVL_DEBG, "logInstantWeather(): Called" );

  // Initialize the date/time string
  nowStr[0] = '\0';

  // Wake up the weather station
  term_flush( wsFD );
  if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
    if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
      if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
      {
        LOGPRINT( LVL_CRIT, "logInstantWeather(): Could not wake "
                  "up the weather station!" );
        return( FAILURE );
      }

  //
  // Start the archive dump
  //
  serialPutLine( wsFD, "LOOP 1\r" );

  if ( ( bytesRead = 
             serialGetData( wsFD, (char *)buffer, 100, 4000L ) ) != 100 )
  {
        LOGPRINT( LVL_CRIT, "logInstantWeather(): Did not receive "
                  "a complete LOOP data record.  Size = %d should "
                  "have been 100", bytesRead );
        return( FAILURE );
  }

  // Grab the current time
  if ( time( &nowTimeT ) >= 0 ) 
  {
    nowTM = localtime( &nowTimeT );
    strftime( nowStr, 80, "%b %d %Y %H:%M:%S  ", nowTM );
  }

  // Cast the buffer to the data structure
  rec = (struct weatherLOOPRevB *)&(buffer[2]);

  fprintf( outFile, 
           "%s\nbarometer = %.2f hg\ninsideHumidity = %d %%\n"
           "insideTemp = %.1f degreesF\noutsideTemp = %.1f degreesF\n" 
           "windSpeed = %d mph\nwindSpeed10MinAvg = %d mph\ndayRain = %d in\n"
           "rainRate = %.2f in/hr\nsolarRad = %d watts/meter^2\n"
           "outsideHumidity = %d %%\n",
           nowStr,
           ( rec->barometer / (double)1000 ), 
           rec->insideHumidity,
           ( rec->insideTemperature / (double)10 ),
           ( rec->outsideTemperature / (double)10 ),
           rec->windSpeed, rec->windSpeed10MinAvg, 
           rec->dayRain, (rec->rainRate/(double)100),
           rec->solarRadiation, rec->outsideHumidity);

  if ( ( rec->windDirection <= 360 && rec->windDirection >= 348 ) ||
       rec->windDirection < 11 ) 
  {
     fprintf( outFile, "windDirection = N" );
  }else if ( rec->windDirection >= 11 && rec->windDirection < 33 )
  {
     fprintf( outFile, "windDirection = NNE" );
  }else if ( rec->windDirection >= 33 && rec->windDirection < 56 )
  {
     fprintf( outFile, "windDirection = NE" );
  }else if ( rec->windDirection >= 56 && rec->windDirection < 78 )
  {
     fprintf( outFile, "windDirection = ENE" );
  }else if ( rec->windDirection >= 78 && rec->windDirection < 101 )
  {
     fprintf( outFile, "windDirection = E" );
  }else if ( rec->windDirection >= 101 && rec->windDirection < 123 )
  {
     fprintf( outFile, "windDirection = ESE" );
  }else if ( rec->windDirection >= 123 && rec->windDirection < 146 )
  {
     fprintf( outFile, "windDirection = SE" );
  }else if ( rec->windDirection >= 146 && rec->windDirection < 168 )
  {
     fprintf( outFile, "windDirection = SSE" );
  }else if ( rec->windDirection >= 168 && rec->windDirection < 191 )
  {
     fprintf( outFile, "windDirection = S" );
  }else if ( rec->windDirection >= 191 && rec->windDirection < 213 )
  {
     fprintf( outFile, "windDirection = SSW" );
  }else if ( rec->windDirection >= 213 && rec->windDirection < 236 )
  {
     fprintf( outFile, "windDirection = SW" );
  }else if ( rec->windDirection >= 236 && rec->windDirection < 258 )
  {
     fprintf( outFile, "windDirection = WSW" );
  }else if ( rec->windDirection >= 258 && rec->windDirection < 281 )
  {
     fprintf( outFile, "windDirection = W" );
  }else if ( rec->windDirection >= 281 && rec->windDirection < 303 )
  {
     fprintf( outFile, "windDirection = WNW" );
  }else if ( rec->windDirection >= 303 && rec->windDirection < 326 )
  {
     fprintf( outFile, "windDirection = NW" );
  }else if ( rec->windDirection >= 326 && rec->windDirection < 348 )
  {
     fprintf( outFile, "windDirection = NNW" );
  }

  fprintf( outFile, "\n" );

  return( SUCCESS );
}



//
// NAME
//   downloadWeatherData - Download from the weather station archive.
//
// SYNOPSIS
//   #include "weather.h"
//
//   int downloadWeatherData( int wsFD, FILE * outFile, int lastRecordOnly );
//
// DESCRIPTION
//   Download data from the Davis weather data logger's archive.
//   This routine can either download all the data and write it
//   to a file or simply the last data record if "lastRecordOnly" 
//   is set to 1.  The outfile must be open and ready for writing
//   and the wsFD file descriptor must be open to the Davis
//   weather station communications port.  If the entire archive
//   is downloaded the archive is cleared before the routine
//   returns.
//
// RETURNS
//   Writes data to the outFile and flushes it at the end.
//   Returns 1 upon success and -1 upon failure.
//
int downloadWeatherData ( int wsFD, FILE * outFile, int lastRecordOnly ) 
{
  unsigned char buffer[268];
  struct weatherDMPRevB lastRecord;
  int i,j, numRecords = 0;
  int bytesRead;
  
  // Say hello
  LOGPRINT( LVL_DEBG, "downloadWeatherData(): Called" );

  // Wake up the weather station
  term_flush( wsFD );
  if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
    if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
      if ( serialChat( wsFD, "\n", "\n", 5000L, "\n" ) < 1 ) 
      {
        LOGPRINT( LVL_CRIT, "downloadWeatherData(): Could not wake "
                  "up the weather station!" );
        return( FAILURE );
      }

  //
  // Start the archive dump
  //
  serialPutLine( wsFD, "DMP\r" );

  // The response is '\r' <ack> 
  serialGetByte( wsFD, (char *)buffer, 1000L );
  serialGetByte( wsFD, (char *)buffer, 1000L );

  //
  //  Read archive pages
  //    1 Byte page index ( since there are 512 pages this goes
  //                        through all indexes twice )
  //    52 Bytes RevB Data Record
  //    52 Bytes RevB Data Record
  //    52 Bytes RevB Data Record
  //    52 Bytes RevB Data Record
  //    52 Bytes RevB Data Record
  //    4  Bytes Unused
  //    2  Bytes Checksum 
  while ( ( bytesRead = 
               serialGetData( wsFD, (char *)buffer, 267, 4000L ) ) > 0 )
  {
    for ( j = 0; j < 5; j++ )
    {
      for ( i = 1; i <= 52; i++ )
      {
        if ( buffer[(j*52)+i] != 255 )
          break;
      }
      if ( i == 53 && buffer[(j*52)+i] == 255 )
      {
        // Empty record
        if ( lastRecordOnly && numRecords > 0 )
        {
          logDMPRecord( outFile, &lastRecord );
        }
        break;
      }else {
        numRecords++;
        if ( lastRecordOnly )
        {
          // Save the last record
          memcpy( &lastRecord, &(buffer[(j*52)+1]), 52 );
        }else
        {
          logDMPRecord( outFile, 
                                 (struct weatherDMPRevB *)&(buffer[(j*52)+1]) );
        }
      }
    }
    if ( j < 5 ) 
    {
      break;
    }
    term_flush( wsFD );
    serialPutLine( wsFD, ackStr );
  }
  // Send ESC to end archive transmission early
  serialPutByte( wsFD, 0x1B );
  // Just in case it sends back a response
  term_flush( wsFD );
  // Give us some time to relax and take in the weather 
  // before sending another command.
  sleep( 1 );

  //
  // Clear the archive if we just downloaded the whole thing.  
  // If we are just peeking at the status then we should
  // leave it intact.
  //
  if ( !lastRecordOnly )
  {
    if ( serialChat( wsFD, "CLRLOG\n", ackStr, 1000L, ackStr ) < 1 )
    {
      LOGPRINT( LVL_CRIT, "downloadWeatherData(): Failed to clear "
                "the archive!" );
      return( FAILURE );
    }
  }
  
  return( SUCCESS );
}



//
// NAME
//   getWSTime - Get the date/time from the weather station.
//
// SYNOPSIS
//   #include "weather.h"
// 
//   struct tm *getWSTime( int fd );
//
// DESCRIPTION
//   Get the date and time in UNIX struct tm format from 
//   the Davis weather station.
//
// RETURNS
//   A pointer to a tm structure or NULL upon
//   failure.
//
struct tm *getWSTime ( int fd ) 
{
  static struct tm wsTime;
  int bytesRead = 0;
  char dataBuff[8];

  if ( serialChat( fd, "GETTIME\r", ackStr, 2000L, ackStr ) < 1 )
  {
    LOGPRINT( LVL_INFO, "getWSTime(): Didn't get ACK after sending GETTIME" );
    return ( (struct tm *)NULL );
  }

  if ( ( bytesRead = serialGetData( fd, dataBuff, 8, 2000L ) ) < 8  ){
    LOGPRINT( LVL_INFO, "getWSTime(): Didn't receive time data from "
              "the weather station!" );
    return ( (struct tm *)NULL );
  }
 
  wsTime.tm_sec  = dataBuff[0]; 
  wsTime.tm_min  = dataBuff[1]; 
  wsTime.tm_hour = dataBuff[2]; 
  wsTime.tm_mday = dataBuff[3]; 
  wsTime.tm_mon  = dataBuff[4] - 1; 
  wsTime.tm_year = dataBuff[5]; 
  //
  // To set or not to set the DST flag?  1 = DST in effect, 0 = not in effect,
  // and -1 means we don't know.  So when we get the time from the instrument
  // the only way to really know if DST is on/off would be to ask the 
  // instrument.  Unfortunately it doesn't return this.  So we have to 
  // make our best guess.  There is a good write up on this which describes
  // the complication that occurs at the time of change. I.e switch occurs
  // at 3am so is a time at 2:30am in the pre-DST or post-DST timeframe?
  // In our case it's not so critical so let's let the computer pick 
  // for us.
  //
  wsTime.tm_isdst = -1;

  return ( &wsTime );
}

 
//
// NAME
//   setWSTime - Set the date/time on the weather station.
//
// SYNOPSIS
//   #include "weather.h"
//
//   int setWSTime( int fd, struct tm *time );
//
// DESCRIPTION
//   Set the date/time on the Davis weather station to
//   the values stored in the UNIX tm format.
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int setWSTime ( int fd, struct tm *time ) {
  char timeData[9];
  char byte;
  unsigned int crc;

  if ( serialChat( fd, "SETTIME\r", ackStr, 1000L, ackStr ) < 1 )
  {
    return ( -1 );
  }

  // Translate the time structure to the weather station bytes
  timeData[0] = (unsigned char)time->tm_sec;
  timeData[1] = (unsigned char)time->tm_min;
  timeData[2] = (unsigned char)time->tm_hour;
  timeData[3] = (unsigned char)time->tm_mday;
  timeData[4] = (unsigned char)time->tm_mon + 1;
  timeData[5] = (unsigned char)time->tm_year;

  // Calculate the CRC
  crc = crc16AddData( (unsigned char *)timeData, 6, 0 );

  // Store the CRC
  timeData[6] = (( crc >> 8 ) & 0xFF);
  timeData[7] = ( crc & 0xFF );

  serialPutData( fd, timeData, 8 );
  if ( serialGetByte( fd, &byte, 1000L ) < 1 || byte != 0x06 ) 
  {
    return ( -1 );
  }

  return ( 1 );
}
  
//
// NAME
//   syncWSTime - Sync date and time on the weather station with our time.
//  
// SYNOPSIS
//   #include "weather.h"
//
// DESCRIPTION
//   Sync a Davis weather station to the system date and time.  
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int syncWSTime ( int fd ) {
 time_t now_t;
 struct tm *now_tm;
 time_t was_t;
 struct tm *was_tm;
 int timeOff = 0;
 
  
 // Get the system time
 now_t = time( NULL );
 now_tm = localtime( &now_t );

 // Get the weather station time
 if ( ( was_tm = getWSTime( fd ) ) == NULL )
 {
   LOGPRINT( LVL_WARN, "syncWSTime(): Could not get weather station time!" );
   return( FAILURE );
 }
 if ( ( was_t = mktime( was_tm ) ) < 0 ) 
 {
   LOGPRINT( LVL_ALRT, "syncWSTime(): The weather station clock data was "
             "garbled...attempting to overwrite it!" );
 }else {
   // See if it's outrageously off
   if ( was_t < now_t && ( now_t - was_t ) > 60 ) 
   { 
     LOGPRINT( LVL_ALRT, "syncWSTime(): The weather station clock was behind "
               "by %d seconds! %d %d", ( now_t - was_t ), now_t, was_t );
     timeOff = 1;
   }else if ( was_t > now_t && ( was_t - now_t ) > 60 ) 
   {
     LOGPRINT( LVL_ALRT, "syncWSTime(): The weather station clock was fast "
               "by %d seconds!", ( was_t - now_t ) );
     timeOff = 1;
   }
 }

  // Set the time
  if ( timeOff ) 
  { 
    if ( setWSTime( fd, now_tm ) < 0 ) 
    {
     return( FAILURE );
    }

    // This is a kludge.  I really wish I didn't have to
    // put delays in here.  It appears that the davis weather
    // station really needs a bit of a delay between operations.
    sleep( 1 );
  
    // Get it again and make sure we set it correctly
    if ( ( was_tm = getWSTime( fd ) ) == NULL )
    {
      LOGPRINT( LVL_WARN, "syncWSTime(): Could not get weather station time!" );
      return( FAILURE );
    }
    was_t = mktime( was_tm );
    if ( abs( was_t - now_t ) > 20 ) 
    {
      LOGPRINT( LVL_ALRT, "syncWSTime(): After attempting to set the weather "
                "station time we are still off by %d seconds!  Failed to set "
                "the time correctly!", ( was_t - now_t ) );
      return( FAILURE );
    }
  }
 
  return( SUCCESS ); 
}


// 
// NAME
//   logDMPRecord - Write a weather record in RevB format to a file.
//
// SYNOPSIS
//   #include "weather.h"
//
//   int logDMPRecord( FILE * outFile, struct weatherDMPRevB *rec );
//
// DESCRIPTION
//   Given a Davis weather station record "rec" write a one line
//   record to a file containing particluar fields.  
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int logDMPRecord( FILE * outFile, struct weatherDMPRevB *rec )
{

  int year, month, day, hour, minute;

  //
  // Date Stamp: offset 0, size 2 
  //  These 16 bits hold the date that the archive was 
  //  written in the following format:
  //    Year (7 bits) | Month (4 bits) | Day (5 bits) 
  //  or
  //    day + ( month * 32 ) + ( ( year - 2000 ) * 512 )
  // 
  year = ( ( rec->dateStamp >> 9 ) & 0x07F ) + 2000; 
  month = ( rec->dateStamp >> 5 ) & 0x0F;
  day = rec->dateStamp & 0x1F;

  //
  // Time on the Vantage that the archive record was
  // written:
  //   (Hour * 100) + minute.
  //
  hour = (int)( rec->timeStamp / (unsigned short)100 );
  minute = rec->timeStamp - ( (unsigned short)hour * (unsigned short)100 );

  fprintf( outFile, 
          "%d/%d/%d %d:%d:00\t%.3f\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t"
          "%.2f\t%d\t%d\n", 
          month, day, year, 
          hour, minute,
          ( rec->barometer / (double)1000 ),
          rec->outsideHumidity,
          rec->averageWindSpeed,
          rec->highWindSpeed,
          rec->prevailingWindDirection,
          (rec->rainfall/ (double)100 ),
          ( rec->insideTemperature / (double)10 ),
          ( rec->outsideTemperature / (double)10 ),
          ( rec->highOutTemperature / (double)10 ),
          ( rec->lowOutTemperature / (double)10 ),
          rec->highSolarRadiation,
          rec->solarRadiation );

  return( 1 );
}

