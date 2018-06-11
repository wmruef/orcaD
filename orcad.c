/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * orcad.c : The main daemon  
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
 * 
 *  TODO: 
 *        Meter wheel calibration?
 *        Cleanup - remove lock file
 *        Initialize CTD19+ correctly
 *          - Synch the CTD time
 *        TEST IOTEST WINCH DIRECTION
 *        Change the debug level of the running program?
 *        Here is a quandry: If we are run from init, how
 *           do we shutdown fast enough so as not to be 
 *           terminated with a SIGKILL?  Init as you know
 *           first sends a SIGTERM then waits 5 seconds
 *           and sends a SIGKILL if the process still exists.
 *           If we take our time and try to download the
 *           weather data we might get SIGKILL'd and lose that
 *           data. ANSWER: use the -t option to telinit so
 *           that you can change the 5 second default to something
 *           more reasonable.  NOTE: This only works when you
 *           are shutting down orcad and not when the OS
 *           is shutting down.
 *        
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h> 
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include "general.h"
#include "term.h"
#include "log.h"
#include "weather.h"
#include "orcad.h"
#include "parser.h"
#include "profile.h"
#include "hardio.h"
#include "buoy.h"
#include "ctd.h"
#include "hydro.h"
#include "util.h"
#include "meterwheel.h"
#include "aquadopp.h"

#define Name "orcad"
extern const char *Version;

#define LCKFILE "/var/run/orcad.pid"
#define LOGFILE "/usr/local/orcaD/logs/orcad"
#define LOGDIR "/usr/local/orcaD/logs"
#define STDERR stderr
#define SHMSZ   27

void processCommandLine(int argc, char *argv[] );

//
// Globals
//   Keep this minimal!  Before adding ask yourself
//   "why am I making this global?...because I am 
//   lazy or because it makes the best design sense?"
//   Then ask yourself "Is it because I don't have
//   any better ideas?".  Then followup with "I
//   do realize that by adding globals I am shooting
//   myself in the foot by making the program harder
//   to maintain.".  And then finish by not adding
//   any more globals.
//


int main(int argc, char *argv[], char *envp[])
{
  int i, ret;
  int pid = -1;
  int pgid = -1;
  sigset_t block;
  sigset_t oblock;
  int weatherFD;
  time_t lastWeatherArchiveTime = -1;
  time_t lastWeatherStatusTime = -1;
  FILE * fpWeather = NULL;
  char weatherStatFile[FILEPATHMAX];

  // Initially point the logging to stderr 
  logFile = stderr;

  // Initialize the datastructures  
  if ( initialize() < 1 ) 
  {
    fprintf( logFile, "%s: Failed to initialize! Exiting.", Name );
    cleanup( FAILURE );
  }

  // Let's just be safe
  WINCH_OFF;

  // Process command line 
  processCommandLine( argc, argv );


  // If we are to be a daemon 
  if ( opts.isDaemon == 1 ) {
    // setup signal handlers 
    //  NOTE: In most cases a signal will terminate
    //        a system call.  There is a way to 
    //        override this behaviour using sigaction
    //        ( for the unix's that have it ) -- see
    //        LPRng::plp_signal or Advanced Programming
    //        in the UNIX Environment, Stevens, 1992
    //
    (void) signal(SIGUSR1, SIG_IGN);
    (void) signal(SIGUSR2, SIG_IGN);
    (void) signal(SIGCHLD, SIG_DFL);
    (void) signal(SIGPIPE, SIG_IGN);
  
    // Unblock signals prior to forking 
    (void) sigemptyset (&block); 
    if (sigprocmask (SIG_SETMASK, &block, &oblock) < 0)
    {
      LOGPRINT( LVL_EMRG, "%s: main(): sigprocmask failed", Name );
      cleanup( FAILURE );
    }
  
    //
    // Fork and create the daemon.  If we don't then we
    // will remain attached to the user's terminal.  
    //
    if ( ( pid = fork() ) < 0 ) 
    {
       LOGPRINT( LVL_EMRG, "%s: main(): fork failed!", Name );
       cleanup( FAILURE );
    }
  
    // Who are we the child or the parent..good question 
    if( pid > 0 )
    {
      // Parent process
      LOGPRINT( LVL_NOTC, "%s: Parent process exiting...", Name );
      exit( SUCCESS );
    }
  
    // "Daemon Child" process 
    LOGPRINT( LVL_NOTC, "%s: Child process alive and well...", Name );
    
    // 
    // Place newly born Daemon Child into it's own proces
    // group so that that it isn't attached to the dying 
    // parent process in any way.
    //
    //
    if ( ( pgid = setpgid(0,getpid()) ) < 0 ) 
    {
      LOGPRINT( LVL_EMRG, "%s: main(): setpgid failed!", Name );
      cleanup( FAILURE );
    }
  
    // Disassociate ourselves with any sort of controlling terminal 
    if ( ( i = open ("/dev/tty", O_RDWR, 0600 ) ) >= 0 ) 
    {
      if( ioctl (i, TIOCNOTTY, (void *)0 ) < 0 )
      {
        LOGPRINT( LVL_EMRG, "%s: main(): TIOCNOTTY failed", Name );
        cleanup( FAILURE );
      }
      (void)close(i);
    }

    // ????
    (void) sigemptyset (&block); 
    if ( sigprocmask (SIG_SETMASK, &block, &oblock) < 0 )
    {
      LOGPRINT( LVL_EMRG, "%s: main(): sigprocmask failed", Name );
      cleanup( FAILURE );
    }
  }

  if ( signal( SIGHUP, cleanup_HUP ) == SIG_ERR )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGHUP handler!", Name );
    cleanup( FAILURE );
  }
  if ( signal( SIGINT, cleanup_INT ) == SIG_ERR )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGINT handler!", Name );
    cleanup( FAILURE );
  }
  if ( signal( SIGQUIT, cleanup_QUIT ) == SIG_ERR )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGQUIT handler!", Name );
    cleanup( FAILURE );
  }
  if ( signal( SIGTERM, cleanup_TERM ) == SIG_ERR )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGTERM handler!", Name );
    cleanup( FAILURE );
  }

  // Run from some standard base directory 
  if( chdir( "/" ) == -1 )
  {
    // Complain
    LOGPRINT( LVL_WARN,
              "%s: main(): Could not change directory to '/'!", Name );
  }


  // Setup formal logging 
  if ( startLogging( LOGFILE, Name, LOG_FILE_ONLY ) < 1 ) 
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Error! Could not start logging!", Name );
    cleanup( FAILURE );
  }

  // Announce ourselves 
  LOGPRINT( LVL_ALRT, "Starting up ( version %s )...", Version );

  // Create a lockfile:
  //    Typically this is stored in /var/run/programname.pid.
  //    Where the contents of the file are the process id
  //    of this program.  The advantage of a lockfile is that
  //    it ensures that two copies of the daemon, orcactrl or
  //    one of each do not run at the same time.  This   
  //    could lead to strange behaviour with the equipment
  //    on the buoy
  //
  if ( createLockFile( LCKFILE, Name ) < 0 ) 
  {
    LOGPRINT( LVL_EMRG, "main(): Could not create a lock file! Exiting." );
    cleanup( FAILURE );
  }

  // Read configuration file
  if ( parseConfigFile( opts.configFileName ) < 0 ) 
  {
    LOGPRINT( LVL_EMRG, "main(): Could not parse config file" );
    cleanup( FAILURE );
  }

  // Initialize the hardware
  if ( initializeHardware() < 0 ) 
  {
    LOGPRINT( LVL_EMRG, "main(): Could not initialize the hardware!" );
    cleanup( FAILURE );
  }

  // Read the last cast file 
  if ( ( opts.lastCastNum = readLastCastInfo() ) < 0 ) 
  {
    LOGPRINT( LVL_EMRG, "main(): Could not read last cast index. Exiting." );
    cleanup( FAILURE );
  }

  // Print out the options as we know them 
  logOpts( logFile );

  //
  // Do main's endless loop here...
  //
  time_t t1 = time(NULL);
  time_t t2;
  long dt;
  short sleep_time = 60;

  // Reset last weather archive time so that we do not 
  // try downloading data until it's ready
  lastWeatherArchiveTime = time(NULL);

  LOGPRINT( LVL_DEBG, "main(): Main schedule loop starting" );

  for (;;) {
    sleep((sleep_time + 1) - (short) (time(NULL) % sleep_time));
    LOGPRINT( LVL_DEBG, "main(): Main schedule loop waking up." );

    t2 = time(NULL);
    dt = t2 - t1;

    // cron usually rechecks the config file for updates
    // ...we could also do this.

    if (dt < -60 * 60 || dt > 60 * 60) 
    {

      t1 = t2;
      LOGPRINT( LVL_WARN, "%s: Time disparity of %d minutes detected!", dt/60 );

    } else if (dt > 0) 
    {

      testJobs( t1, t2, opts.missions );
      runJobs( opts.missions );

      if ( hasSerialDevice( DAVIS_WEATHER_STATION ) > 0 )
      {
        if ( ( weatherFD = getDeviceFileDescriptor( 
                                  DAVIS_WEATHER_STATION ) ) > 0 )
        {
          if ( ( t2 - lastWeatherArchiveTime ) > 
               (opts.weatherArchiveDownloadPeriod * 60)  )
          {
            // Time to download all the weather data from the
            // archive to a MET file.
  
  
            if ( ( fpWeather = openNewWeatherFile() ) != NULL )
            {
              if ( ( ret = downloadWeatherData( weatherFD, fpWeather, 0 ) ) < 0 )
              {
                LOGPRINT( LVL_WARN, "%s: Failed to download weather archive. "
                          "Return code = %d.", ret );
              }
              fclose( fpWeather );
            }else 
            {
              LOGPRINT( LVL_WARN, "%s: Failed to open a weather archive data "
                        "file!" );
            }
            lastWeatherArchiveTime = t2;
          }
          // Currently hard-coded to 10 minutes
          if ( ( t2 - lastWeatherStatusTime ) > 
               ( 10 * 60)  )
          {
            // Time to obtain a weather report.  Save the
            // most recent weather data to a status file for
            // download by our customers.
            snprintf( weatherStatFile, FILEPATHMAX, "%s/%s", opts.dataDirName, 
                      opts.weatherStatusFilename );
            if( ( fpWeather = fopen( weatherStatFile, "w" ) ) != NULL )
            {
              // For some reason the downloadWeatherData routine
              // is unable to consistently download the last record
              // of the archive for the purposes creating an
              // instant weather service.  Instead we will try using
              // the logInstantWeather routine as a workaround.
              //if ( ( ret = downloadWeatherData( weatherFD, fpWeather, 1 ) ) < 0 )
              if ( ( ret = logInstantWeather( weatherFD, fpWeather ) ) < 0 )
              {
                LOGPRINT( LVL_WARN, "%s: Failed to download weather status. "
                          "Return code = %d.", ret );
              }
              fclose( fpWeather );
            }else 
            {
              LOGPRINT( LVL_WARN, "%s: Failed to open the weather status file!" );
            }
            lastWeatherStatusTime = t2;
          }
        }
      } // if ( hasSerialDevice( DAVIS_WEATHER_STATION )...
      sleep(5);
      t1 = t2;
    }

    // Check to see if log file is too large ( Currently 100KB )
    if ( rotateLargeLog( LOGFILE, 100000 ) < 0 ) 
    {
      cleanup( FAILURE );
    }

  }

  // Should never ever get here. But....
  cleanup( FAILURE );
}



int initialize() {
  static char defaultConfigFile[] = "/usr/local/orcaD/orcad.cfg";

  // Initialize ourselves
  opts.debugLevel = 4;
  opts.minDepth = 2;
  opts.maxDepth = 2;
  opts.parkingDepth = 2;
  opts.isDaemon = 1;
  opts.lastCastNum = 0;
  opts.inCritical = 0;
  opts.weatherArchiveDownloadPeriod = 1440;
  opts.weatherDataPrefix = "MET";
  opts.weatherStatusFilename = "weather-status.dat";
  opts.ctdCalPA1 = 248.24555;
  opts.ctdCalPA2 = -6.524518E-2;
  opts.ctdCalPA3 = 5.430179E-8;
  opts.solarCalibrationConstant = 0;
  opts.solarMillivoltResistance = 0;
  opts.solarADMultiplier = 0;
  opts.meterwheelCFactor = 1.718213058;
  opts.configFileName = defaultConfigFile;
  opts.dataFilePrefix = '\0';
  opts.dataDirName[0] = '\0';
  opts.dataSubDirName[0] = '\0';
  opts.serialPorts = '\0';
  opts.missions = '\0';

  return( SUCCESS );
}

int runJobs ( struct mission *mPtr )
{
  unsigned int sleepSec;
  int ret, hydroFD, hydroDeviceType;
  int aquaFD = 0;
  short nJobs = 0;
  struct mission *currMission;
  FILE *outFile;
  char dataLogFile[FILEPATHMAX];

  currMission = mPtr;
  while ( currMission != NULL ) {
    LOGPRINT( LVL_DEBG, "runJobs(): Considering mission: %s", 
              currMission->name);
    if ( currMission->cl_Pid == -1 ) 
    {

      LOGPRINT( LVL_NOTC, "runJobs(): Running mission: %s", currMission->name);

      // WMR 10/16/13: Toggle hydro wire power on. Now that the buoy's
      //               no longer have battery packs underwater we only
      //               need to power up the hydrowire during a profile.
      HYDRO_ON;
      sleepSec = 5;
      LOGPRINT( LVL_INFO,
      "profile(): Powering up the hydrowire...and sleeping for %d secs", sleepSec );
      while ( ( sleepSec = sleep( sleepSec ) ) > 0 ) { /* nothing */ }
 

      // Do profile
      if ( ( ret = profile( currMission ) ) < 0 ) 
      {
        LOGPRINT( LVL_ALRT, "runJobs(): Mission %s profile returned: %d", 
                  currMission->name, ret );
      }

      // Save the CTD Data in good times and bad:
      //     - as long as the error occured after
      //       the profile setup.
      if ( ret >= 0 || ret < ESTUP )
      {
        if ( ( outFile = openNewCastFile() ) != NULL )
        {
  
          sprintf( dataLogFile,"%s/%s%04ld.HEX", opts.dataSubDirName,
          opts.dataFilePrefix, opts.lastCastNum );
          LOGPRINT( LVL_NOTC, "runJobs(): Creating %s",
                               dataLogFile );
          hydroDeviceType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroDeviceType );
          downloadHydroData( hydroDeviceType, hydroFD, outFile );
          fclose( outFile );
        }else {
          LOGPRINT( LVL_CRIT, "runJobs(): Could not open open" 
                              " a new HEX file! Data not saved!" ); 
        }
  
        // Save the aquadopp data if we have one
        if ( hasSerialDevice( AQUADOPP ) > 0 )
        {
          if ( ( aquaFD = getDeviceFileDescriptor( AQUADOPP ) ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "runJobs(): Could not open up the " 
                                "aquadopp serial port!" );
          }else { 
            if ( downloadAquadoppFiles( aquaFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "runJobs(): Failed to download aquadopp files!" );
            }
          }
        }
      }

      nJobs++;
      currMission->cl_Pid = 0;

      //
      // Sync the time with the CTD just for good measure
      //
      hydroDeviceType = getHydroWireDeviceType();
      hydroFD = getDeviceFileDescriptor( hydroDeviceType );
      if ( syncHydroTime( hydroDeviceType, hydroFD ) < 0 )
      {
        LOGPRINT( LVL_ALRT, "runJobs(): Failed to sync CTD time!" );
      }

      // WMR 10/16/13: Toggle hydro wire power off. Now that the buoy's
      //               no longer have battery packs underwater we only
      //               need to power up the hydrowire during a profile.
      HYDRO_OFF;      
      LOGPRINT( LVL_INFO,
              "profile(): Powering off the hydrowire.");

      LOGPRINT( LVL_NOTC, "runJobs(): Completed mission: %s\n", 
                          currMission->name);
    }
    currMission = currMission->nextMission;
  }
  return ( nJobs );
}

int testJobs (time_t t1, time_t t2, struct mission *mPtr)
{
  short nJobs = 0;
  time_t t;
  struct mission *currMission;

  /* Find jobs > t1 and <= t2 */
  for (t = t1 - t1 % 60; t <= t2; t += 60) {
    if (t > t1) {
      struct tm *tp = localtime(&t);
      //for each mission
      currMission = mPtr;
      while ( currMission != NULL ) {
        LOGPRINT( LVL_DEBG, 
                  "testJobs(): Considering mission: %s", currMission->name);

        if (    currMission->startMinsList[ tp->tm_min ]
             && currMission->startHoursList[ tp->tm_hour ]
             && (    currMission->startDaysOfMonthList[ tp->tm_mday ]
                  || currMission->startDaysOfWeekList[ tp->tm_wday ] )
             && currMission->startMonthsList[ tp->tm_mon ] )
        {
 
          LOGPRINT( LVL_DEBG, 
                    "testJobs(): Mission ready to run: %s", currMission->name);
          if (currMission->cl_Pid == 0) {
            currMission->cl_Pid = -1;
            ++nJobs;
          }
        }

        currMission = currMission->nextMission;

      }
    }
  }
  return (nJobs);
}



static char *options[] = {
 "usage: %s [-v][-d #][-h][-c conf_file][-f]\n",
 " Options\n",
 " -c conf_file - configuration file ( default: /etc/orcad.cfg )\n",
 " -d #         - set debug level and flags ( default: 0 )\n",
 " -v           - show version info\n",
 " -f           - stay in the foreground, do not run as a daemon\n",
 " -h           - prints this information\n",
 0,
};
 


void usage (void)
{
  int i;
  char *s;
  for( i = 0; (s = options[i]); ++i ){
    if( i == 0 ){
      fprintf( STDERR, s, Name );
    } else {
      fprintf( STDERR, "%s", s );
    }
  }
  fprintf( STDERR, "\n Version: %s\n", Version );
  exit(1);
}



void processCommandLine (int argc, char *argv[] )
{
  int option = 0;
  while ((option = getopt (argc, argv, "h?c:d:vf" )) != EOF){
    switch (option) {
      case 'c': 
                opts.configFileName = 
                      (char * )malloc( (strlen( optarg )+1) * sizeof( char ) );
                strcpy( opts.configFileName, optarg );
                break;
      case 'd': 
                sscanf( optarg, "%d", &opts.debugLevel );
                break;
      case 'h':
        usage();
      case 'v': 
        fprintf( STDERR, "%s\n", Version ); 
        cleanup( FAILURE );
      case 'f': 
                opts.isDaemon = 0;
                break;
      default: usage(); break;
    }
  }
}


void cleanup (int passed_signal)
{
  int ret = 0;
  int weatherFD = 0;
  sigset_t block;
  sigset_t oblock;
  FILE * fpWeather = NULL;

  // Block all signals 
  (void) sigfillset (&block);
  sigprocmask (SIG_SETMASK, &block, &oblock);

  // Check to see if we are in a critical
  // section of the code. I.e the winch is
  // on!
  if ( opts.inCritical == 1 )
  {
    // Assume that if the winch is on that we do
    // not have to initialize the SMART I/O system.
    // Just go ahead and shut off the winch for
    // God sakes!
    EMERGENCY_WINCH_OFF;
    // Important to place this after the attempt....if only to ensure
    // that the emergency off completes before anything else happens.
    LOGPRINT( LVL_EMRG, "cleanup(): Caught exit signal ( %d ) while in "
                        "critical loop!!!!!!  Attemping to shutdown "
                        "the winch...", passed_signal );
  }else {
    LOGPRINT( LVL_EMRG, "cleanup(): Caught exit signal ( %d )!  " 
                        "Attempting to shutdown cleanly...", passed_signal );
  }

  // Shutdown powered devices
  if ( getHydroWireDeviceType() > -1 )
  {
    // There appears to be a hydro device...shut down it's power feed
    LOGPRINT( LVL_INFO, "cleanup(): Turning off hydrowire power" );
    HYDRO_OFF;
  }

 
  if ( hasSerialDevice( DAVIS_WEATHER_STATION ) > 0 )
  {
    if ( ( weatherFD = getDeviceFileDescriptor( 
                           DAVIS_WEATHER_STATION ) ) > 0 )
    {
      // Before shutting down the weather station download what remains
      // of the volatile data archive.
      if ( ( fpWeather = openNewWeatherFile() ) != NULL )
      {
        if ( ( ret = downloadWeatherData( weatherFD, fpWeather, 0 ) ) < 0 )
        {
          LOGPRINT( LVL_WARN, "cleanup(): Failed to download weather archive. "
                              "Return code = %d.", ret );
        }
        fclose( fpWeather );
      }else 
      {
        LOGPRINT( LVL_WARN, "cleanup(): Failed to open a weather archive data "
                            "file!" );
      }
    }else {
      LOGPRINT( LVL_WARN, "cleanup(): Failed to open a serial connection to "
                          "the weather station.  Shouldn't it already be "
                          "open? " );
    }
    // Turn off
    LOGPRINT( LVL_INFO, "cleanup(): Turning off weather station "
              "power" );
    WEATHER_OFF;
  }

  if ( hasSerialDevice( AGO_METER_WHEEL_COUNTER ) > 0 )
  {
    // Turn it on
    LOGPRINT( LVL_INFO, "cleanup(): Turning off meter wheel counter "
              "power" );
    METER_OFF;
  }

  // TODO: shut off all io

  // TODO: close all file descriptors

  // Say our last goodbye
  LOGPRINT( LVL_ALRT, "cleanup(): All lights are shut off, the stove is off " 
                      " and the cat is fed.  Leaving for good.  Cya!" );
  exit( passed_signal );
}

void cleanup_USR1 ()
{
  cleanup(SIGUSR1);
}

void cleanup_HUP ()
{
  cleanup(SIGHUP);
}

void cleanup_INT ()
{
  cleanup(SIGINT);
}

void cleanup_QUIT ()
{
  cleanup(SIGQUIT);
}

void cleanup_TERM ()
{
  cleanup(SIGTERM);
}


