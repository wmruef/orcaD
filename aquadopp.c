/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * aquadopp.c : Aquadopp routines
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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include "general.h"
#include "log.h"
#include "orcad.h"
#include "serial.h"
#include "general.h"
#include "util.h"
#include "aquadopp.h"


// Useful info for time routines
static const char *const monthNamesList[] = {
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec",
  NULL
};


//
// Strings containing useful Aquadopp constants
//    -  ACK ACK
//    -  NoACK NoACK 
//
static char dblAck[] = { 0x06, 0x06, 0x00 };


//
// NAME
//   initAquadopp - Initialize the Aquadopp for normal operations
//
// SYNOPSIS
//   #include "aquadopp.c"
//
//   int initAquadopp( int aquadoppFD );
//
// DESCRIPTION
//   The Aquadopp  blah blah blah
//
// RETURN VALUE
//   The function returns 1 upon success and a -1 upon failure.
//
int initAquadopp( int aquadoppFD ) 
{
  // Not much to do right now except sync our times
  return( syncAquadoppTime( aquadoppFD ) );
}


//
// NAME
//   initAquadopp - Initialize the Aquadopp for normal operations
//
// SYNOPSIS
//   #include "aquadopp.c"
//
//   int initAquadopp( int aquadoppFD );
//
// DESCRIPTION
//   The Aquadopp  blah blah blah
//
// RETURN VALUE
//   The function returns 1 upon success and a -1 upon failure.
//

int syncAquadoppTime( int aquadoppFD ) 
{
  time_t now_t;
  struct tm *now_tm;
  time_t was_t;
  struct tm *was_tm;
  int timeOff = 0;

  // Say hello
  LOGPRINT( LVL_DEBG, "syncAquadoppTime(): Called" );

  // Get the system time
  now_t = time( NULL );
  now_tm = localtime( &now_t );


  // Get the aquadopp time
  if ( ( was_tm = getAquadoppTime( aquadoppFD ) ) == NULL )
  {
    LOGPRINT( LVL_WARN, "syncAquadoppTime(): Could not get "
              "aquadopp time!" );
    return( FAILURE );
  }

  if ( ( was_t = mktime( was_tm ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "syncAquadoppTime(): The aquadopp clock data was "
              "garbled!  Giving up." );
    return( FAILURE );
  }else {
    // See if it's outrageously off
    if ( was_t < now_t && ( now_t - was_t ) > 60 )
    {
      LOGPRINT( LVL_ALRT, "syncAquadoppTime(): The aquadopp clock was behind "
                "by %d seconds! %d %d", ( now_t - was_t ), now_t, was_t );
      timeOff = 1;
    }else if ( was_t > now_t && ( was_t - now_t ) > 60 )
    {
      LOGPRINT( LVL_ALRT, "syncAquadoppTime(): The aquadopp clock was fast "
                "by %d seconds!", ( was_t - now_t ) );
      timeOff = 1;
    }
  }

  if ( timeOff )
  {
    if ( setAquadoppTime( aquadoppFD, now_tm ) < 0 )
    {
      LOGPRINT( LVL_ALRT, "syncAquadoppTime(): Could not set the aquadopp time!" );
      //return( FAILURE );
    }

    // Get the system time
    now_t = time( NULL );
    now_tm = localtime( &now_t );


    // Get the aquadopp time
    if ( ( was_tm = getAquadoppTime( aquadoppFD ) ) == NULL )
    {
      LOGPRINT( LVL_WARN, "syncAquadoppTime(): Could not get "
                "aquadopp time 2nd time!" );
      return( FAILURE );
    }

    // and make sure we set it correctly
    was_t = mktime( was_tm );
    if ( abs( was_t - now_t ) > 20 )
    {
      LOGPRINT( LVL_ALRT, "syncAquadoppTime(): After attempting to set the "
                "aquadopp time we are still off by %d seconds!  Failed to set "
                "the time correctly!", ( was_t - now_t ) );
      //return( FAILURE );
    }
  }

  return( SUCCESS );
}


//
// NAME
//   getAquadoppPrompt - Obtain a command prompt from a Aquadopp 
//
// SYNOPSIS
//   #include "aquadopp.c"
//
//   int getAquadoppPrompt( int aquadoppFD );
//
// DESCRIPTION
//   The Aquadopp may be in one of three modes of operation:
//   Command Mode, Power Down, or Data Collection Mode. 
//   This routine ensures the instrument is in Command Mode
//   before continuing.
//   
// RETURN VALUE
//   The function returns 1 if succesful or 
//   a -1 upon failure.
//
int getAquadoppPrompt ( int aquadoppFD )
{
  char buffer[256];
  int retries = 0;
  int atPrompt = 0;
  int bufSize = 0;

  // Say hello
  LOGPRINT( LVL_VERB, "getAquadoppPrompt(): Called" );


  // Try to get the command prompt
  do { 
    //
    // See if we can talk to the Aquadopp 
    //
    term_flush( aquadoppFD );
  
    //
    // Sending a break on an Aquadopp is a bit of an artform
    // and not very well documented in our release of the manual.
    // The idea is you send "@@@@@@" wait 100ms and follow it up
    // with the logical "K1W%!Q" ( case sensitive ).  Depending
    // one which mode you are in you will get different responses.
    // The end result is that you end up in command mode though.
    // The tech writes:
    //   "The reason for the varying outputs is that the @'s 
    //    has the purpose to take the instrument out of sleep
    //    mode or wait mode. If the instrument is in sleep 
    //    mode when you send the @'s,  this is treated as a 
    //    break in itself. If the instrument is up and running,
    //    the @'s ensure that the instrument will interpret the
    //    "K1W%1Q" string which will be recognised as a break. 
    //    It may also happen that the "NORTEK" string comes 
    //    twice if the instrument was in sleep mode when the
    //    break was sent."
    //
    // Send the break prefix
    serialPutLine( aquadoppFD, "@@@@@@" );
    // This may be forever to the aquadopp since it only requires
    // that the delay between break strings is > 100ms.
    usleep( 100 );
    // Send the break suffix
    serialPutLine( aquadoppFD, "K1W%!Q" );

    static const char CONFIRM_PRMPT[] = "Confirm:";
    static const char COMMAND_PRMPT[] = "Command mode";
  
    // Wait for response
    if ( (bufSize = serialGetLine( aquadoppFD, buffer, 256, 500, dblAck )) >= 0 )
    {
      //if( strstr( buffer, "Confirm:" ) ) 
      if( memmem( buffer, bufSize, CONFIRM_PRMPT, strlen(CONFIRM_PRMPT) ) ) 
      {
        // Send the confirmation
        serialPutLine( aquadoppFD, "MC" );
        // Wait for response
        if ( (bufSize = serialGetLine( aquadoppFD, buffer, 256, 500, dblAck )) >= 0 ) 
        {
          //if ( strstr( buffer, "Command mode" ) ) 
          if ( memmem( buffer, bufSize, COMMAND_PRMPT, strlen(COMMAND_PRMPT) ) ) 
          {
            atPrompt = 1;
          }else
          {
            // String doesn't contain "Command mode" - Bad
          }
        }else 
        {
          // Could not get a response with a double ACK - Bad
        }
      //}else if ( strstr( buffer, "Command mode" ) ) 
      }else if ( memmem( buffer, bufSize, COMMAND_PRMPT, strlen(COMMAND_PRMPT) ) ) 
      {
        atPrompt = 1;
      }else 
      {
        // String doesn't contain "Command mode" - Bad
      }
    }else 
    {
      // Could not get a response with a double ACK - Bad
    }
  }while ( ! atPrompt && retries++ < 5 );


  if ( atPrompt )
  {
    return( SUCCESS );
  }else
  {
    return( FAILURE );
  }
}


//
// NAME
//   stopLoggingAquadopp - Cease Aquadopp logging
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int stopLoggingAquadopp( int aquadoppFD );
//
// DESCRIPTION
//   Cease Nortek Aquadopp logging. This is accomplished
//   by entering command mode.
//
// RETURNS
//   Returns 1 for success and -1 for failure. 
//
int stopLoggingAquadopp ( int aquadoppFD )
{
  // Say hello
  LOGPRINT( LVL_VERB, "stopLoggingAquadopp(): Called" );

  return ( getAquadoppPrompt( aquadoppFD ) );
}


//
// NAME
//   startLoggingAquadopp - Initiate Aquadopp logging
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int startLoggingAquadopp( int aquadoppFD );
//
// DESCRIPTION
//   Initiate Nortek Aquadopp logging through the use of the
//   "SD" command.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int startLoggingAquadopp ( int aquadoppFD )
{
  // Say hello
  LOGPRINT( LVL_VERB, "startLoggingAquadopp(): Called" );

  if ( getAquadoppPrompt( aquadoppFD ) )
  {
    if ( serialChat( aquadoppFD, "SD", dblAck , 1000L, dblAck ) < 1 ) 
      if ( serialChat( aquadoppFD, "SD", dblAck , 1000L, dblAck ) < 1 ) 
      {
        LOGPRINT( LVL_WARN, "startLoggingAquadopp(): Failed "
                  "to write SD command to Aquadopp!");
        return( FAILURE );
      }
  }else 
  {
    LOGPRINT( LVL_WARN, "startLoggingAquadopp(): Failed "
              "to get Aquadopp command prompt!");
    return( FAILURE );
  }

  return( SUCCESS );
}


int littleToBigEndianWord ( char * data )
{
  int retVal = 0;
  retVal = (data[1] * 256) + data[0];
  return ( retVal );
}

char bcdToDec ( char bcdVal )
{
  return ( ((bcdVal / 16) * 10) + (bcdVal % 16) );
}

char decToBCD ( char decVal )
{
  return ( (decVal / 10) * 0x10 + (decVal % 10) );
}

//
// NAME
//   getAquadoppTime - Get the date/time from the Aquadopp 
//
// SYNOPSIS
//   #include "aquadopp.h"
// 
//   struct tm *getAquadoppTime( int aquadoppFD );
//
// DESCRIPTION
//   Get the date and time in UNIX struct tm format from 
//   the Aquadopp
//
// RETURNS
//   A pointer to a tm structure or NULL upon
//   failure.
//
struct tm *getAquadoppTime ( int aquadoppFD ) 
{
  static struct tm aquadoppTime;
  int bytesRead = 0;
  int i = 0;
  char dataBuff[12];

  // Say hello
  LOGPRINT( LVL_VERB, "getAquadoppTime(): Called" );

  // Normalize the Aquadopp state by getting an "S>" prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining an command prompt failed 
    LOGPRINT( LVL_WARN, "getAquadoppTime(): All attempts "
              "at obtaining a command prompt failed." );
    return( (struct tm *)NULL );
  }

  // Send the RC command to obtain the BCD encoded time/date data
//       ! ( bytesRead == 10 && dataBuff[0] == 0x52 && dataBuff[1] == 0x43 && 
  serialPutLine( aquadoppFD, "RC" );
  bytesRead = serialGetData( aquadoppFD, dataBuff, 12, 6000L );
  if ( ! ( bytesRead == 8 && dataBuff[6] == 0x06 && dataBuff[7] == 0x06 ) &&
       ! ( bytesRead == 10&& dataBuff[8] == 0x06 && dataBuff[9] == 0x06) )
  {
    int v;
    for ( v = 0; v < bytesRead; v++ )
    {
      printf("RC - dataBuff[%d] = %d\n", v, dataBuff[v] );
    }
    LOGPRINT( LVL_WARN, "getAquadoppTime(): Did not get a "
              "complete response to the RC command bytesRead=%d", bytesRead );
    for ( i = 0; i < bytesRead; i++ )
    {
      LOGPRINT( LVL_DEBG, "getAquadoppTime(): dataBuff[%d]: %d\n", 
                i, dataBuff[i] );
    }
    return( (struct tm *)NULL );
  }

  char *dataBuffPtr = &dataBuff[0];
  if ( bytesRead  == 10 )
    dataBuffPtr = &dataBuff[2];
 
  LOGPRINT( LVL_DEBG, "getAquadoppTime(): dataBuffPtr[0-5]: %d %d %d %d %d %d", 
            dataBuffPtr[0], dataBuffPtr[1], dataBuffPtr[2], dataBuffPtr[3],
            dataBuffPtr[4], dataBuffPtr[5] );

  // Setup the data structure
  aquadoppTime.tm_min  = bcdToDec( dataBuffPtr[0] ); 
  aquadoppTime.tm_sec  = bcdToDec( dataBuffPtr[1] ); 
  aquadoppTime.tm_mday = bcdToDec( dataBuffPtr[2] ); 
  aquadoppTime.tm_hour = bcdToDec( dataBuffPtr[3] ); 
  aquadoppTime.tm_year = bcdToDec( dataBuffPtr[4] ) + 100; 
  aquadoppTime.tm_mon  = bcdToDec( dataBuffPtr[5] ) - 1; 
  aquadoppTime.tm_isdst = -1;

  LOGPRINT( LVL_DEBG, "getAquadoppTime(): Time: %s", asctime( &aquadoppTime ));
  return ( &aquadoppTime );
}

//
// NAME
//   setAquadoppTime - Set the date/time on the Aquadopp 
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int setAquadoppTime( int aquadoppFD, struct tm *time );
//
// DESCRIPTION
//   Set the date/time on the Aquadopp to
//   the values stored in the UNIX tm format.
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int setAquadoppTime ( int aquadoppFD, struct tm *time ) {
  char dataBuff[12];

  // Normalize the Aquadopp state by getting a command prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "setAquadoppTime(): All attempts "
              "at obtaining a command prompt failed." );
    return( FAILURE );
  }

  // Setup the data structure
  // NOTE: We have noticed that the Aquadopp is very picky about the timing
  // of the transmission of command bytes.  If we called:
  //     serialPutLine( aquadoppFD, "SC" );
  // and then 
  //     serialPutData( aquadoppFD, dataBuff, 6 );
  // the delay between the SC and the parameter data is enough to 
  // confuse the Aquadopp. It does seem to work reliably if you 
  // send all the data at once though:
  dataBuff[0] = 'S';
  dataBuff[1] = 'C';
  dataBuff[2] = decToBCD( time->tm_min ); 
  dataBuff[3] = decToBCD( time->tm_sec ); 
  dataBuff[4] = decToBCD( time->tm_mday ); 
  dataBuff[5] = decToBCD( time->tm_hour ); 
  dataBuff[6] = decToBCD( time->tm_year - 100 ); 
  dataBuff[7] = decToBCD( time->tm_mon + 1 ); 
  dataBuff[8] = 0x00; 

  LOGPRINT( LVL_DEBG, "setAquadoppTime(): dataBuff[0-5]: %d %d %d %d %d %d", dataBuff[2], dataBuff[3], dataBuff[4], dataBuff[5], dataBuff[6], dataBuff[7] );

  serialPutData( aquadoppFD, dataBuff, 8 );
  if ( serialChat( aquadoppFD, "", dblAck, 5000L, dblAck ) < 1 ) 
  {
    LOGPRINT( LVL_WARN, "setAquadoppTime(): Failed sending "
              "SC command and data!" );
    return( FAILURE );
  }

  return ( SUCCESS );
}

//
// NAME
//   displayAquadoppConfig - Read the configuration info and display
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int displayAquadoppConfig( int aquadoppFD );
//
// DESCRIPTION
//   Reads the Aquadopp configuration files using the "GA" command,
//   casts the return buffer to the three structures ( hardware
//   configuration, head configuration, and user configuration ), and
//   logs the results.
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int displayAquadoppConfig ( int aquadoppFD ) {
  int i, bytesRead;
  char dataBuff[1024];
  static struct tm aquadoppTime;
  struct hardwareConfig *hardwareRec;
  //struct headConfig *headRec;
  struct userConfig *userRec;

  // Normalize the Aquadopp state by getting a command prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "displayAquadoppConfig(): All attempts "
              "at obtaining a command prompt failed." );
    return( FAILURE );
  }

  LOGPRINT( LVL_DEBG, "displayAquadoppConfig(): ");


  serialPutLine( aquadoppFD, "GA" );
  bytesRead = serialGetData( aquadoppFD, dataBuff, 784, 6000L );
  
  // Newer aquadopps appear to echo the command in the response
  char *dataBuffPtr = &dataBuff[0];
  if ( dataBuff[0] == 'G' && dataBuff[1] == 'A' )
    dataBuffPtr = &dataBuff[2];

  if ( bytesRead != 784 || ( dataBuffPtr[0] != 0xA5 && dataBuffPtr[1] != 0x05 ) )
  {
    LOGPRINT( LVL_WARN, "displayAquadoppConfig(): Did not get a "
              "complete response to the GA command bytesRead=%d", bytesRead );
    for ( i = 0; i < bytesRead; i++ )
    {
      LOGPRINT( LVL_DEBG, "displayAquadoppConfig(): dataBuff[%d]: %d\n", 
                i, dataBuff[i] );
    }
    return( FAILURE );
  }

  // Cast the buffer to the data structures
  hardwareRec = (struct hardwareConfig *)(dataBuffPtr);
  // Unused at this time
  //headRec = (struct headConfig *)&(dataBuff[48]);
  userRec = (struct userConfig *)&(dataBuffPtr[272]);

  aquadoppTime.tm_sec  = bcdToDec( userRec->clockDeploy[1] ); 
  aquadoppTime.tm_min  = bcdToDec( userRec->clockDeploy[0] ); 
  aquadoppTime.tm_hour = bcdToDec( userRec->clockDeploy[3] ); 
  aquadoppTime.tm_mday = bcdToDec( userRec->clockDeploy[2] ); 
  aquadoppTime.tm_mon  = bcdToDec( userRec->clockDeploy[5] ) - 1; 
  aquadoppTime.tm_year = bcdToDec( userRec->clockDeploy[4] ) + 100; 
  aquadoppTime.tm_isdst = -1;

  LOGPRINT( LVL_ALRT, "displayAquadoppConfig(): Hardware Config:\n"
                      "                            Serial Number = %x %x"
                      " %x %x %x %x %x %x %x %x %x %x %x %x\n"
                      "                            PIC Version = %d\n"
                      "                            HW Version = %d\n"
                      "                            RecSize = %d bytes\n"
                      "                            FW Version = %x %x %x %x\n"
                      "                         User Config:\n"
                      "                            DeployName = %s\n"
                      "                            ClockDeploy = %s\n",
                      hardwareRec->serialNo[0],hardwareRec->serialNo[1],
                      hardwareRec->serialNo[2],hardwareRec->serialNo[3],
                      hardwareRec->serialNo[4],hardwareRec->serialNo[5],
                      hardwareRec->serialNo[6],hardwareRec->serialNo[7],
                      hardwareRec->serialNo[8],hardwareRec->serialNo[9],
                      hardwareRec->serialNo[10],hardwareRec->serialNo[11],
                      hardwareRec->serialNo[12],hardwareRec->serialNo[13], 
                      littleToBigEndianWord( hardwareRec->picVersion ),
                      littleToBigEndianWord( hardwareRec->hwRevision ), 
                      littleToBigEndianWord( hardwareRec->recSize ) * 65536,
                      hardwareRec->fwVersion[0], hardwareRec->fwVersion[1],
                      hardwareRec->fwVersion[2], hardwareRec->fwVersion[3],
                      userRec->deployName, asctime( &aquadoppTime ) );                    

  return ( SUCCESS );
}


//
// NAME
//   displayAquadoppFAT - Display the recorder directory contents
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int displayAquadoppFAT( int aquadoppFD );
//
// DESCRIPTION
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int displayAquadoppFAT ( int aquadoppFD ) 
{
  int    FATIndex = 0;
  struct aquadoppFAT buffer[32];

  // Normalize the Aquadopp state by getting a command prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "displayAquadoppFAT(): All attempts "
              "at obtaining a command prompt failed." );
    return( FAILURE );
  }

  serialPutLine( aquadoppFD, "RF" );
  if ( serialGetLine( aquadoppFD, (char *)buffer, 
                      sizeof( struct aquadoppFAT ) * 32, 
                      500, dblAck ) >= 0 ) 
  {

    while ( buffer[ FATIndex ].filenamePrefix[0] != '\0' )
    {
      LOGPRINT( LVL_DEBG, "displayAquadoppFAT(): filename = %s seq=%x status=%x "
                " start= %x-%x-%x-%x end= %x-%x-%x-%x",
                buffer[ FATIndex ].filenamePrefix, 
                buffer[ FATIndex ].filenameSeq,
                buffer[ FATIndex ].status,
                buffer[ FATIndex ].startAddress[0],
                buffer[ FATIndex ].startAddress[1],
                buffer[ FATIndex ].startAddress[2],
                buffer[ FATIndex ].startAddress[3],
                buffer[ FATIndex ].stopAddress[0], 
                buffer[ FATIndex ].stopAddress[1], 
                buffer[ FATIndex ].stopAddress[2], 
                buffer[ FATIndex ].stopAddress[3] );
      FATIndex++;
    }
  }
  return ( SUCCESS );
}      
 
//
// NAME
//   clearAquadoppFAT - Erase the recorder contents
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int clearAquadoppFAT( int aquadoppFD );
//
// DESCRIPTION
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int clearAquadoppFAT ( int aquadoppFD ) 
{
  int    bytesRead = 0;
  char   fileBuff[129];

  // Normalize the Aquadopp state by getting a command prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "clearAquadoppFAT(): All attempts "
              "at obtaining a command prompt failed." );
    return( FAILURE );
  }

  term_flush( aquadoppFD );
  // Command to erase the recorder ( easier on the CF memory than format )
  static char FO_Erase[] = { 0x46, 0x4f, 0x12, 0xd4, 0x1e, 0xef, 0x00 };
  // Command to format the recorder
  //static char FO_Format[] = { 0x46, 0x4f, 0x4d, 0x52, 0x54, 0x41, 0x00 };

  serialPutLine( aquadoppFD, FO_Erase );

  if (( bytesRead = serialGetLine( aquadoppFD, fileBuff, 128, 20000L, dblAck )) > 0 )
  {
    if ( ! memmem( fileBuff, bytesRead, dblAck, 2 ) )
    {
      LOGPRINT( LVL_WARN, "clearAquadoppFAT(): Failed sending "
                          "FO (erase) command and data! Missing dblAck "
                          "in response!" );
      return( FAILURE );
    } 
  }else {
    LOGPRINT( LVL_WARN, "clearAquadoppFAT(): Failed sending "
                        "FO (erase) command and data! Missing response!" );
    return( FAILURE );
  }

  return ( SUCCESS );
}


//
// NAME
//   downloadAquadoppFiles - Download the entire aquadopp archive
//
// SYNOPSIS
//   #include "aquadopp.h"
//
//   int downloadAquadoppFiles( int aquadoppFD );
//
// DESCRIPTION
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int downloadAquadoppFiles ( int aquadoppFD ) 
{
  char   dataLogFile[FILEPATHMAX];
  int    FATIndex = 0;
  int    bytesRead = 0;
  int    retVal = 0;
  int    retries = 0;
  int    fileSuffixNum = 0;
  char   fileBuff[129];
  char   cmdArr[16];
  struct aquadoppFAT buffer[32];
  struct stat statBuff;
  FILE   *fpl;

  // Normalize the Aquadopp state by getting a command prompt
  if ( getAquadoppPrompt( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "clearAquadoppFAT(): All attempts "
              "at obtaining a command prompt failed." );
    return( FAILURE );
  }

  // Create/set the data directory variable.
  updateDataDir();
  
  serialPutLine( aquadoppFD, "RF" );
  if ( serialGetLine( aquadoppFD, (char *)buffer, 
                      sizeof( struct aquadoppFAT ) * 32, 
                      500, dblAck ) >= 0 ) 
  {
    while ( buffer[ FATIndex ].filenamePrefix[0] != '\0' )
    {
      LOGPRINT( LVL_DEBG, "downloadAquadoppFiles(): filename = %s seq=%x status=%x "
                " start= %x-%x-%x-%x end= %x-%x-%x-%x",
                buffer[ FATIndex ].filenamePrefix, 
                buffer[ FATIndex ].filenameSeq,
                buffer[ FATIndex ].status,
                buffer[ FATIndex ].startAddress[0],
                buffer[ FATIndex ].startAddress[1],
                buffer[ FATIndex ].startAddress[2],
                buffer[ FATIndex ].startAddress[3],
                buffer[ FATIndex ].stopAddress[0], 
                buffer[ FATIndex ].stopAddress[1], 
                buffer[ FATIndex ].stopAddress[2], 
                buffer[ FATIndex ].stopAddress[3] );

      long startAddr = 0;
      long stopAddr = 0;
      startAddr = ( (buffer[ FATIndex ].startAddress[0] << 24) | 
                    (buffer[ FATIndex ].startAddress[1] << 16) |
                    (buffer[ FATIndex ].startAddress[2] << 8) |
                    (buffer[ FATIndex ].startAddress[3]) );
      stopAddr =  ( (buffer[ FATIndex ].stopAddress[0] << 24) |
                    (buffer[ FATIndex ].stopAddress[1] << 16) |
                    (buffer[ FATIndex ].stopAddress[2] << 8) |
                    (buffer[ FATIndex ].stopAddress[3]) );

      if ( ( stopAddr - startAddr ) > 100000 )
      {
          LOGPRINT( LVL_WARN, "downloadAquadoppFiles(): Warning - unusually large aquadopp file: filename = %s seq=%x status=%x "
                " start= %x-%x-%x-%x end= %x-%x-%x-%x size=%ld",
                buffer[ FATIndex ].filenamePrefix, 
                buffer[ FATIndex ].filenameSeq,
                buffer[ FATIndex ].status,
                buffer[ FATIndex ].startAddress[0],
                buffer[ FATIndex ].startAddress[1],
                buffer[ FATIndex ].startAddress[2],
                buffer[ FATIndex ].startAddress[3],
                buffer[ FATIndex ].stopAddress[0], 
                buffer[ FATIndex ].stopAddress[1], 
                buffer[ FATIndex ].stopAddress[2], 
                buffer[ FATIndex ].stopAddress[3], ( stopAddr - startAddr ) );
      }
      
      if ( buffer[ FATIndex ].startAddress[0] != 
              buffer[ FATIndex ].stopAddress[0] ||
           buffer[ FATIndex ].startAddress[1] != 
              buffer[ FATIndex ].stopAddress[1] ||
           buffer[ FATIndex ].startAddress[2] != 
              buffer[ FATIndex ].stopAddress[2] ||
           buffer[ FATIndex ].startAddress[3] != 
              buffer[ FATIndex ].stopAddress[3]  )
      { 
        // Open a file
        sprintf( dataLogFile,"%s/%s%04ld.AQD", opts.dataSubDirName,
                 opts.dataFilePrefix, opts.lastCastNum );

        while(    ( retVal = stat(dataLogFile, &statBuff ) ) == 0
               && retries++ < 32 )
        {
          fileSuffixNum++;
          sprintf( dataLogFile,"%s/%s%04ld_%02d.AQD", opts.dataSubDirName,
                 opts.dataFilePrefix, opts.lastCastNum, fileSuffixNum );
        }
        if ( retVal == 0 )
        {
          LOGPRINT( LVL_WARN, "downloadAquadoppFiles(): Failed to create "
                    "a unique filename on filesystem.  Last attempt produced "
                    "%s and it already exists!", dataLogFile );
          return( FAILURE );
        } 
  
        if((fpl = fopen(dataLogFile,"w")) == NULL)
        {
           LOGPRINT( LVL_CRIT, "downloadAquadoppFiles(): Could not open new "
                    "data file = %s", dataLogFile );
           return( FAILURE );
        }
  
        LOGPRINT( LVL_ALWY, "downloadAquadoppFiles: Saving aquadopp file to "
                  "data file = %s", dataLogFile );
  
  
        LOGPRINT( LVL_DEBG, "downloadAquadoppFiles: Saving configuration " 
                            "data.. " );
  
        // Clear the port
        term_flush( aquadoppFD );
  
        // Normalize the Aquadopp state by getting a command prompt
        if ( getAquadoppPrompt( aquadoppFD ) < 1 )
        {
          // All attempts at obtaining a command prompt failed 
          LOGPRINT( LVL_WARN, "clearAquadoppFAT(): All attempts "
                    "at obtaining a command prompt failed." );
          return( FAILURE );
        }

        cmdArr[0] = 'F';
        cmdArr[1] = 'C';
        cmdArr[2] = 0x00;
        cmdArr[3] = 0x00;
        cmdArr[4] = 0x00;
        cmdArr[5] = 0x00;
        cmdArr[6] = 0x00;
        cmdArr[7] = 0x00;
        cmdArr[8] = 0x00;
        cmdArr[9] = FATIndex;
        serialPutData( aquadoppFD, cmdArr, 10 );
    
        /*
         * Timming Issues...
        // Send the command to download a record file
        serialPutLine( aquadoppFD, "FC" );
        // Give it the FAT index
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, 0x00 );
        serialPutByte( aquadoppFD, FATIndex );
        */
  
        // Read/write the data
        // NOTE: Currently this leave the ACK ACK in between the data sets.
        //       I believe this should be removed so as not to interfere
        //       with NORTEKS conversion software.
        int totalBytesRead = 0;
        while ( ( bytesRead = 
                 serialGetLine( aquadoppFD, fileBuff, 128, 500L, dblAck ) ) > 0 )
        {
          // if ( bytesRead > 1 )
          // Characterize the current block
          //if ( fileBuff[ bytesRead-1 ] == 0x06 )
          //{
          //  bytesRead--;
          //  prevAcksFound = 1;
          //}
          //if ( fileBuff[ bytesRead-1 ] == 0x06 )
          //{
          //  bytesRead--;
          //  prevAcksFound++;
          //}
          fwrite( fileBuff, 1, bytesRead, fpl ); 
          totalBytesRead += bytesRead;
        }

        // Clear the port
        term_flush( aquadoppFD );
  
        // Normalize the Aquadopp state by getting a command prompt
        if ( getAquadoppPrompt( aquadoppFD ) < 1 )
        {
          // All attempts at obtaining a command prompt failed 
          LOGPRINT( LVL_WARN, "clearAquadoppFAT(): All attempts "
                    "at obtaining a command prompt failed." );
          return( FAILURE );
        }

        LOGPRINT( LVL_DEBG, "downloadAquadoppFiles: Saving measurement " 
                          "data.. " );

        cmdArr[0] = 'R';
        cmdArr[1] = 'D';
        cmdArr[2] = buffer[ FATIndex ].startAddress[0];
        cmdArr[3] = buffer[ FATIndex ].startAddress[1];
        cmdArr[4] = buffer[ FATIndex ].startAddress[2];
        cmdArr[5] = buffer[ FATIndex ].startAddress[3];
        cmdArr[6] = buffer[ FATIndex ].stopAddress[0];
        cmdArr[7] = buffer[ FATIndex ].stopAddress[1];
        cmdArr[8] = buffer[ FATIndex ].stopAddress[2];
        cmdArr[9] = buffer[ FATIndex ].stopAddress[3];
        serialPutData( aquadoppFD, cmdArr, 10 );
 
        /*
         * Timming Issues...
        // Send the command to download a record file
        serialPutLine( aquadoppFD, "RD" );
        // Give it the start memory location
        serialPutByte( aquadoppFD, buffer[ FATIndex ].startAddress[0] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].startAddress[1] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].startAddress[2] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].startAddress[3] );
        // Give it the stop memory location - little endian
        serialPutByte( aquadoppFD, buffer[ FATIndex ].stopAddress[0] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].stopAddress[1] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].stopAddress[2] );
        serialPutByte( aquadoppFD, buffer[ FATIndex ].stopAddress[3] );
        */
      
        // Read/write the data
        // NOTE: Currently this leave the ACK ACK at the end of the file
        //       I believe this should be removed so as not to interfere
        //       with NORTEKS conversion software.
        while ( ( bytesRead = 
                 serialGetLine( aquadoppFD, fileBuff, 128, 500L, dblAck ) ) > 0 )
        {
          fwrite( fileBuff, 1, bytesRead, fpl ); 
          totalBytesRead += bytesRead;
        }
  
        // Close the file.
        fclose( fpl );
      }else {
        LOGPRINT( LVL_WARN, "downloadArchive(): WARNING: Aquadop file has "
                            "zero length ( same start/stop position ) "
                            "skipping!" );
      }

      FATIndex++;
    }
  }else {
    LOGPRINT( LVL_WARN, "downloadArchive(): Could not read Aquadopp FAT. "
                        "Nothing returned from RF command. " );
    return( FAILURE );
  }

  if ( clearAquadoppFAT( aquadoppFD ) < 1 )
  {
    // All attempts at obtaining a command prompt failed 
    LOGPRINT( LVL_WARN, "downloadAquadoppFiles(): Failed to erase "
              "the aquadopp archive." );
    return( FAILURE );
  }

  // Send power down comamnd
  if ( serialChat( aquadoppFD, "PD", dblAck , 1000L, dblAck ) < 1 ) 
    if ( serialChat( aquadoppFD, "PD", dblAck , 1000L, dblAck ) < 1 ) 
    {
      LOGPRINT( LVL_WARN, "downloadAquadoppFiles(): Warning! Failed "
                "to write PD ( power down ) command to Aquadopp!");
    }
 
  return( SUCCESS );
}
 
