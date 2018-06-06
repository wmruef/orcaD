/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * log.c : Routines to assist with program logging
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "general.h"
#include "orcad.h"
#include "log.h"


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
// NAME
//   startLogging - initialize the program logging routines
//
// SYNOPSIS
//   #include "log.h"
// 
//   int startLogging( char *fileName, char *name, int dest );
//
// DESCRIPTION
//   This routine is used to configure the logging system.
//   It does not strictly need to be called before using 
//   routines like logPrint.  The fileName is opened up
//   for use and stored in the global "logDest" for use
//   by logPrint.  Likewise the program name is stored
//   in a global "progName".  The dest parameter is 
//   used to control the spliting of the logPrint
//   messages to the logFile, the screen or both ( see
//   log.h for valid settings for this parameter ).
//  
// RETURNS
//   1 on Success
//  -1 on Failure
//
int startLogging ( char *fileName, char *name, int dest ) {

  progName = (char *)malloc(  sizeof( char ) * (strlen( name ) + 1) );
  strcpy( progName, name );
  logDest = dest;
  if( ( logFile = fopen( fileName, "a+" ) ) == NULL ){
    logFile = stdout;
    return ( FAILURE );
  }

  return ( SUCCESS );

}


//
// NAME
//   logPrint - Print a message to a logging device
//
// SYNOPSIS
//   #include "log.h"
// 
//   int logPrint( int level, char *message, ... );
//
// DESCRIPTION
//   Log a line to the log file.
// 
// RETURNS
//   1 on Success
//  -1 on Failure
//
void logPrint ( int level, char *message, ... )
{
  va_list ap;
  time_t nowTimeT;
  struct tm *nowTM;
  char nowStr[80];

  if ( logFile != NULL && level <= opts.debugLevel ) {
    nowStr[0] = '\0';
    va_start( ap, message );  
    if ( time( &nowTimeT ) >= 0 ) {
      nowTM = localtime( &nowTimeT );
      if ( strftime( nowStr, 80, "%b %d %Y %H:%M:%S  ", nowTM ) >= 0 ) {
        if ( progName == NULL ) 
        {
          fprintf( logFile, "%s ", nowStr );
        }else {
          fprintf( logFile, "%s%s: ", nowStr, progName );
        }
        if ( logDest == LOG_SCREEN_AND_FILE && 
             logFile != stdout && logFile != stderr ) 
        {
          printf("%s%s: ", nowStr, progName );
        }
      }   
    }
    vfprintf( logFile, message, ap );
    fprintf( logFile, "\n" );
    if ( logDest == LOG_SCREEN_AND_FILE && 
         logFile != stdout && logFile != stderr ) 
    {
      vfprintf( stdout, message, ap );
      fprintf( stdout, "\n" );
    }
    fflush( logFile );
    sync();
    va_end(ap);
  }
}


//
// NAME
//   rotateLargeLog - Rotate log file if it's size is large
//
// SYNOPSIS
//   #include "log.h"
//
//   int rotateLargeLog( char *fileName, int size );
//
// DESCRIPTION
//   Rotate the log file if it's size is greater than 
//   the "size" parameter.
//
// RETURNS
//   1 on Success
//  -1 on Failure
//
int rotateLargeLog ( char *fileName, int size ) 
{
  struct stat buf;

  //   Stat File
  if ( stat( fileName, &buf) < 0 ) 
  {
    LOGPRINT( LVL_WARN, "rotateLargeLog(): Could not stat file %s!",
              fileName );
    return( FAILURE );
  }
  
  // Check size
  if ( buf.st_size > size ) 
  {
    LOGPRINT( LVL_ALWY, "rotateLargeLog(): Rotating log file because it's "
              "large: %d bytes!", buf.st_size );
    if ( logRotate( fileName ) < 0 ) 
    {
      return( FAILURE );
    } 
  }

  return( SUCCESS );
}


//
// NAME
//   logRotate - Rotate log file
//
// SYNOPSIS
//   #include "log.h"
//
//   int logRotate( char *fileName );
//
// DESCRIPTION
//   Rotate the log file.  This involves closing the
//   old one, create a new name for it, renaming it,
//   and starting a new log file for the program.
//
// RETURNS
//   1 on Success
//  -1 on Failure
//
int logRotate ( char *fileName )
{
  FILE *logFilePtr;
  char buffer[128];
  char month[4];
  char *newFileName;
  struct stat buf;
  int day, year, hour, i;

  // read file and parse date
  if ( ( logFilePtr = fopen( fileName , "r" ) ) != NULL ) 
  {
    if ( fgets(buffer, 128, logFilePtr) != NULL ) 
    {
      // NOTE This will fail on older log files which didn't have
      //      the year field! Won't be a problem after all the
      //      buoy's rotate once with this version.
      if ( (i = sscanf( buffer, "%3s %d %d %d", month, &day, 
                        &year, &hour )) == 4 ) 
      { 
        for (i = 0; monthNamesList[i]; ++i) 
        {
          if ( strncasecmp( month, monthNamesList[i], 
                            strlen(monthNamesList[i])) == 0) 
          {
            break;
          }
        }
  
        //     create rotated filename
        newFileName = (char *)malloc( strlen(fileName) + 15 );
        if ( snprintf( newFileName, strlen(fileName)+15, "%s.%d%.2d%.2d%.2d", 
                       fileName, year, i, day, hour ) > 0 )
        {
          // rename file if the new name doesn't already exist!
          if ( stat( newFileName, &buf ) < 0 ) 
          { 
            rename( fileName, newFileName );
          }
        }
        free( newFileName );
      } 
    }
    fclose( logFilePtr );
  } // if ( ( logFilePtr = fopen(...

  // close log file
  if ( fclose( logFile ) >= 0 )
  {
    //     open log file   
    if( ( logFile = fopen( fileName, "a+" ) ) == NULL ){
      logFile = stdout;
      return ( FAILURE );
    }
  } 

  return( SUCCESS );

}
