/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * util.c : Utility routines  
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
#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include "util.h"
#include "general.h"
#include "orcad.h"
#include "log.h"


// 
// NAME
//   updateDataDir - Create a new data directory if necessary.
//
// SYNOPSIS
//    #include "util.h"
//
//    int updateDataDir();
//
// DESCRIPTION
//   This routine uses the global opts.dataDirName to locate
//   the top-level data storage directory for the program.  Data
//   files are organized into subfolders, one per year-month 
//   ( yyyymm name format ).  The current in-use subdirectory name
//   ( i.e. /usr/local/orcaD/data/200501 ) is saved in the global
//   opts.dataSubDirName.  This routine checks that the current
//   year-month folder exists and if not creates it.
//
// RETURNS
//    1   :  Success
//   -1   :  Failure
//
int updateDataDir ()
{
  char   newDataSubDirName[FILEPATHMAX];
  struct tm * now_tm;
  struct stat dirstat;
  time_t now_t;

  // Get time of day
  now_t = time(NULL);
  now_tm = (struct tm *)localtime( &now_t );
 
  //
  // Determine if we need a new data subdirectory
  //
  snprintf( newDataSubDirName, FILEPATHMAX, "%s/%4d%02d", 
            opts.dataDirName, ( now_tm->tm_year+1900 ),
            ( now_tm->tm_mon + 1 ) );
  if ( stat( newDataSubDirName, &dirstat ) < 0 ) 
  {
    // Could not stat the directory!
    if ( errno != ENOENT ) 
    {
      LOGPRINT( LVL_CRIT, "openNewDataFile(): Unknown data directory "
                "stat error = %d!", errno );
      return( FAILURE );
    }

    if ( mkdir( newDataSubDirName, (mode_t)555 ) < 0 )
    {
      // Could not make directory!
      LOGPRINT( LVL_CRIT, "openNewDataFile(): Could not make a new data sub "
                "directory = %s", newDataSubDirName );
      return( FAILURE );
    }
      
  }

  // Save new directory name
  if ( strcmp( newDataSubDirName, opts.dataSubDirName ) != 0 )
    strcpy( opts.dataSubDirName, newDataSubDirName );

  return( SUCCESS );
}


// 
// NAME
//   openNewCastFile - Create a new data file to store CTD data for one cast.
//
// SYNOPSIS
//    #include "util.h"
//
//    int openNewCastFile();
//
// DESCRIPTION
//   This routine calls the updateDataDir() to create ( if necessary )
//   a new data subdirectory.  It then increments the opts.lastCastNum
//   global and stores it in a file by calling writeLastCastInfo().
//   Lastly the file is created using the opts.dataFilePrefix global
//   along with the cast number and "HEX" extension.
//
// RETURNS
//    1   :  Success
//   -1   :  Failure
//
FILE * openNewCastFile ()
{
  char   dataLogFile[FILEPATHMAX];
  struct tm * now_tm;
  time_t now_t;
  FILE   *fpl;

  updateDataDir();

  // Get time of day
  now_t = time(NULL);
  now_tm = (struct tm *)localtime( &now_t );

  // Increment the cast number
  opts.lastCastNum++;
  // Save it to a file
  if ( writeLastCastInfo( opts.lastCastNum ) < 0 )
  {
    LOGPRINT( LVL_CRIT, "openNewCastFile(): Could not write the "
              "last cast index!" );
    return( (FILE *)NULL );
  }
  LOGPRINT( LVL_INFO, "openNewCastFile(): Starting Cast Number %d at %s", 
            opts.lastCastNum, asctime( now_tm ) );
  sprintf( dataLogFile,"%s/%s%04ld.HEX", opts.dataSubDirName, 
           opts.dataFilePrefix, opts.lastCastNum );
  //
  // SET UP NEW DATA FILE
  //
  if((fpl = fopen(dataLogFile,"w")) == NULL) 
  {
     // Do Something
     LOGPRINT( LVL_INFO, "openNewCastFile(): Could not open new "
               "data file = %s", dataLogFile ); 
  }

  return( fpl );
}

// 
// NAME
//   openNewWeatherFile - Create a new data file to store weather data.
//
// SYNOPSIS
//    #include "util.h"
//
//    int openNewWeatherFile();
//
// DESCRIPTION
//   This routine calls the updateDataDir() to create ( if necessary )
//   a new data subdirectory.  It then opens up a new file with the
//   name YYYYMMDDhhmm.MET where Y=Year, M=Month, D=day, h=hour, 
//   m=minute.  
//
// RETURNS
//    1   :  Success
//   -1   :  Failure
//
FILE * openNewWeatherFile()
{
  char   dataLogFile[FILEPATHMAX];
  struct tm * now_tm;
  time_t now_t;
  FILE   *fpl;

  updateDataDir();

  // Get time of day
  now_t = time(NULL);
  now_tm = (struct tm *)localtime( &now_t );

  sprintf( dataLogFile,"%s/%s%04d%02d%02d%02d%02d.MET", opts.dataSubDirName,
           opts.weatherDataPrefix, 
           now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, 
           now_tm->tm_hour, now_tm->tm_min );
  //
  // SET UP NEW DATA FILE
  //
  if((fpl = fopen(dataLogFile,"w")) == NULL) 
  {
     // Do Something
     LOGPRINT( LVL_INFO, "openNewWeatherFile(): Could not open new "
               "data file = %s", dataLogFile ); 
  }

  return( fpl );
}


//
// NAME
//   readLastCastInfo - Open and read the cast index file.
//
// SYNOPSIS
//   #include "util.h"
//
//   long readLastCastInfo();
//
// DESCRIPTION
//   The last cast index file is a file which stores
//   the state of the cast index number.  A file separate
//   from the config file was choosen to simplify the
//   parsing and modification of this simple parameter.
//   The number stored in this file represents the last
//   index used.  The file name is stored in the global
//   parameter opts.dataDirName/CASTIDXFILE.
//
// RETURNS
//    1 Upon Success
//   -1 Upon failure
//
long readLastCastInfo ()
{
   FILE *castIdxFile;
   char castIdxFileName[FILEPATHMAX];
   long retValue;

   snprintf( castIdxFileName, FILEPATHMAX,
             "%s/%s", opts.dataDirName, CASTIDXFILE );
   if ( ( castIdxFile = fopen( castIdxFileName , "r" ) ) == NULL ) {
     return( -1 );
   }

   if ( fscanf( castIdxFile, "%ld", &retValue ) == 1 ) {
     fclose( castIdxFile );
     return( retValue );
   }

   fclose( castIdxFile );
   return( -1 );
}


//
// NAME
//   writeLastCastInfo - Write the last cast index to a file.
//
// SYNOPSIS
//   #include "util.h"
//
//   int writeLastCastInfo( long castIdx );
//
// DESCRIPTION
//   Write the number of the last cast index used to a file.
//   The file name is stored in the global parameter 
//   opts.dataDirName/CASTIDXFILE.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int writeLastCastInfo ( long castIdx )
{
   FILE *castIdxFile;
   char castIdxFileName[FILEPATHMAX];

   snprintf( castIdxFileName, FILEPATHMAX,
             "%s/%s", opts.dataDirName, CASTIDXFILE );
   if ( ( castIdxFile = fopen( castIdxFileName , "w+" ) ) == NULL ) {
     return( -1 );
   }
   fprintf( castIdxFile, "%ld\n", castIdx );
   fclose( castIdxFile );
   return( 1 );
}


//
// NAME
//   createLockFile - Create a lockfile to avoid running duplicate daemons
//
// SYNOPSIS
//   #include "util.h"
//
//   int createLockFile( char *lckFileName, char * prgName );
//
// DESCRIPTION
//   Create a file containing the PID of the running program and
//   named lckFileName.  If this file exists and is locked by another
//   process then log the reason for failure and return -1.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int createLockFile ( char *lckFileName, char *prgName ) {
  char str[LINEBUFFLEN];
  int n = 0;
  int pid = -1;
  int lckstat = -1;
  int lckfd = -1;

  // Step 1: Read the file if it exists and get the PID
  if( (lckfd = open( lckFileName, O_NOCTTY|O_APPEND|O_RDWR|O_CREAT, 0 ) )< 0 ){
    LOGPRINT( LVL_WARN,
              "createLockFile(): open of '%s' failed.  It's possible "
              "that another %s is already running or a permissions "
              "problem exists with this file.", 
              lckFileName, prgName);
    return( FAILURE );
  }
  if ( lckfd >= 0 ) {
    // Lockfile exists. We should investigate further as to why
    // it does.
    LOGPRINT(LVL_DEBG, "createLockFile(): Lockfile exists -- open sucessful");
    if( lseek( lckfd, 0, SEEK_SET ) == -1 ){
      LOGPRINT(LVL_WARN, 
               "createLockFile(): lseek failed. Check that no other "
               "copy of %s is running and then try removing the "
               "lockfile ( %s ) before restarting the program.", 
               prgName, lckFileName );
      close( lckfd );
      return( FAILURE );
    }
    str[0] = 0;
    if( (n = read( lckfd, str, LINEBUFFLEN-1 ) ) < 0 ){
      LOGPRINT( LVL_WARN, 
                "createLockFile(): failed to read from lockfile ( %s ).  "
                "Check that no other copy of %s is running and then "
                "try removing the lockfile before restarting "
                "the program.", 
                lckFileName, prgName );
      close( lckfd );
      return( FAILURE );
    }
    str[n] = 0;
    pid = atoi( str );
    LOGPRINT( LVL_DEBG, 
              "createLockFile(): Lockfile maintains that the PID is %d", pid);

    lckstat = flock( lckfd, LOCK_EX|LOCK_NB );
    LOGPRINT(LVL_DEBG, "createLockFile(): flock of returned %d", lckstat);
    if ( lckstat < 0 ) {
      LOGPRINT( LVL_WARN, 
                "createLockFile(): failed to lock the lockfile ( %s ).  "
                "Check that no other copy of %s is running and then "
                "try removing the lockfile before restarting "
                "the program.",
                lckFileName, prgName );
      close( lckfd );
      return( FAILURE );
    }
   
    if( lseek( lckfd, 0, SEEK_SET ) == -1 ){
      LOGPRINT( LVL_WARN,
                "createLockFile(): lseek failed. Check that no other "
                "copy of %s is running and then try removing the "
                "lockfile ( %s ) before restarting the program.", 
                prgName, lckFileName );
      close( lckfd );
      return( FAILURE );
    }
    if( ftruncate( lckfd, 0 ) ){
      LOGPRINT(LVL_WARN, "createLockFile(): ftruncate failed" );
      close( lckfd );
      return( FAILURE );
    }
    if ( snprintf( str, LINEBUFFLEN, "%d", getpid() ) >= LINEBUFFLEN ) {
      LOGPRINT( LVL_WARN, 
                "createLockFile(): could not convert pid to string!\n" );
      close( lckfd );
      return( FAILURE );
    }
    if ( write( lckfd, str, strlen(str) ) == -1 )  {
      LOGPRINT(LVL_WARN, "createLockFile(): write failed" );
      close( lckfd );
      return( FAILURE );
    }
  }
  return ( SUCCESS );
}


