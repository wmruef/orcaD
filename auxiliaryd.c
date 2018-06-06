/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * auxiliaryd.c : The auxiliary sensor logging daemon  
 *
 * Created: November 2014
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
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <auxiliaryd.h>
#include <hardio.h>
#include <buoy.h>
#include <log.h>
#include <util.h>
#include <general.h>
#include <orcad.h>
#include <parser.h>
#include <serial.h>

#define FAILURE -1
#define LCKFILE "/var/run/auxiliaryd.pid"
#define Name "auxiliaryd"
#define LOGFILE "/usr/local/orcaD/logs/auxiliaryd"
#define STDERR stderr
#define AUXBUFFLEN 256

extern const char *Version;
//struct ftdi_context *ftdic = NULL;
FILE * fpAuxiliary = NULL;
time_t timeLastArchiveCreated;
time_t timeLastSampled;


/*auxiliaryd:

  A deamon for reading a data stream or to poll a 
  set of auxiliary sensors.  

  Current supported devices:
 
    - Satlantic SeaFET ocean pH sensor

   SeaFET Flow
   ----------------
    - preconfigure SeaFET for polled mode prior to deployment
    - during deployment:
       - define sampling schedule
       - send SeaFET any character (we use "!!!!!") to wake up
       - within 5 seconds of sending wake up command, send "s" command to sample
       - SeaFET will echo back "s" if sampling command is received 
       - read SeaFET data line
       - write data line to AUX file
       - SeaFET automatically enters sleep state

*/




/* 
 * Usage Screen Data
 */
static char *options[] = {
 "usage: %s [-v][-d #][-h][-c conf_file][-f]\n",
 " Options\n",
 " -c conf_file - configuration file ( default: /usr/local/orcaD/orcad.cfg )\n",
 " -d #         - set debug level and flags ( default: 0 )\n",
 " -v           - show version info\n",
 " -f           - stay in the foreground, do not run as a daemon\n",
 " -h           - prints this information\n",
 0,
};


/*
 * Print usage data to screen
 */
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


/*
 * MAIN PROGRAM
 */
int main(int argc, char **argv)
{
  // Future Expansion
  //int auxiliaryDeviceType = -1;
  int i;
  sigset_t block;
  sigset_t oblock;
  int pid = -1;
  int pgid = -1;

  // Initialize the datastructures
  if ( initialize() < 1 )
  {
    fprintf( logFile, "%s: Failed to initialize! Exiting.", Name );
    cleanup( FAILURE );
  }

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

  // 
  // Install signal handlers
  //
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
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGQUIT handler!", 
              Name );
    cleanup( FAILURE );
  }
  if ( signal( SIGTERM, cleanup_TERM ) == SIG_ERR )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Could not install SIGTERM handler!", 
              Name );
    cleanup( FAILURE );
  }

  // Run from some standard base directory 
  if( chdir( "/" ) == -1 )
  {
    // Complain
    LOGPRINT( LVL_WARN,
              "%s: main(): Could not change directory to '/'!", Name );
  }

  // Start logging
  logFile = stderr;
  if ( startLogging( LOGFILE, Name, LOG_FILE_ONLY ) < 1 ) {
    LOGPRINT( LVL_EMRG, "%s: main(): Error! Could not start logging!", Name );
    cleanup( FAILURE );
  }

  // Initialize the IO subsystem
  if ( initializeIO() < 0 )
  {
    LOGPRINT( LVL_EMRG, "%s: main(): Error! Could not initialize IO subsystem!", Name );
    cleanup( FAILURE );
  }

  // Initialize the term library
  int r = term_lib_init();
  if ( r < 0 )
  {
    LOGPRINT( LVL_ALRT,
              "main(): term_init failed: %s",
              term_strerror(term_errno, errno));
    return( FAILURE ); 
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
    LOGPRINT( LVL_EMRG, "main(): Could not parse config file %s", opts.configFileName );
    cleanup( FAILURE );
  }

  // Print out the options as we know them
  logOpts( logFile );

  if ( ( fpAuxiliary = fopen("/usr/local/orcaD/data/tmpAuxFile", "w" ) ) == NULL )
  {     
    LOGPRINT( LVL_EMRG, "Could not open up a new auxiliary file!");
    cleanup( FAILURE );
  }

  if ( hasSerialDevice( SEAFETPH ) > 0 )
  {
    wsSEAFET();

  }

  return( 1 );

} // main();



/*************************************************************************/



void wsSEAFET( )
{
  int auxPort = -1;
  // Initialize the time
  time_t nowTimeT;
  struct tm *nowTM;
  tzset();
  time( &nowTimeT );  
  nowTM = localtime( &nowTimeT );
  char dataLogFile[FILEPATHMAX];
  char buffer[AUXBUFFLEN];
  int bytesRead = 0;

  char *fileHeader = NULL;
 
  if ( hasSerialDevice( SEAFETPH  ) > 0 )
  {
    static char *auxDesc = "# Satlantic SeaFET Ocean pH Sensor\n";
    fileHeader = realloc( fileHeader,80 * sizeof(char));
    if ( fileHeader == NULL )
    {
      LOGPRINT( LVL_EMRG, "wsSEAFET(): Could not allocate memory for auxDesc!");
      cleanup( FAILURE );
    }
    strcat( fileHeader, auxDesc );

    auxPort = getDeviceFileDescriptor( SEAFETPH );
    if ( ! auxPort )
    {
      LOGPRINT( LVL_EMRG, "wsSEAFET(): Could not open up seafet device!");
      cleanup( FAILURE );
    }
  }

  static char *fieldDesc = 
        "# fields: month, day, year, hour, minute, pH\n";
  fileHeader = realloc( fileHeader,(strlen(fileHeader)+strlen(fieldDesc) + 1)*sizeof(char) );
  if ( fileHeader == NULL )
  {
    LOGPRINT( LVL_EMRG, "wsSEAFET(): Could not allocate memory for fieldDesc!");
    cleanup( FAILURE );
  }
  strcat( fileHeader, fieldDesc );

  // Write header to file
  fprintf( fpAuxiliary,"%s", fileHeader);

  // Last minute initializations
  time( &nowTimeT );  
  timeLastArchiveCreated = nowTimeT;
  timeLastSampled = nowTimeT;
  nowTM = localtime( &nowTimeT );

  LOGPRINT( LVL_ALRT, "Logging started" );

  //
  // Collection Loop
  //
  for (;;)
  {
    // Record the time
    time( &nowTimeT );  
    nowTM = localtime( &nowTimeT );

    // Check to see if we need to create AUX archive
    if ( ( nowTimeT - timeLastArchiveCreated ) > (1440 * 60) )
    {
      // Time to close the AUX file and reopen a new one
      fclose( fpAuxiliary );

      updateDataDir();
      
      // Move the temporary file into a new AUX file
      sprintf( dataLogFile,"%s/%s%04d%02d%02d%02d%02d.AUX", 
           opts.dataSubDirName,
           opts.auxiliaryDataPrefix,
           nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
           nowTM->tm_hour, nowTM->tm_min );

      LOGPRINT( LVL_ALRT, "Creating %s.", dataLogFile );
      rename( "/usr/local/orcaD/data/tmpAuxFile", dataLogFile );

      if ( ( fpAuxiliary = fopen("/usr/local/orcaD/data/tmpAuxFile", "w" ) ) 
            == NULL )
      {     
        LOGPRINT( LVL_ALRT, "Could not open up a new auxiliary file!");
        cleanup( FAILURE );
      }
      fprintf( fpAuxiliary,"%s", fileHeader);
      fflush( fpAuxiliary );

      timeLastArchiveCreated = nowTimeT;
    }
    if ( ( nowTimeT - timeLastSampled ) > (opts.auxiliarySamplePeriod * 60) )
    {
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 1: time to take a sample!");
      // Read from the auxiliary device(s)
      //  -- SeaFET pH

      // SEAFET Data
      // Full ASCII MOde: max number of characters in Full (long) ASCII frame is 197
      // Example from manual: SATPHA0001,2014083,13.0932646,7.80829,7.78879,20.5221,20.5038,32.6027,4.669,10.523,0.05975991,
      //	-0.85879952,1.13056064,11.326,114,0.5,4.882,9.602,6.106,5.903,100,100,-0.95693927,0x0000,7<CR><LF>
      //
      // Example from sensor: SATPHA0217,2014317,23.2585735,6.53811,6.41548,18.8480,nan,nan,nan,nan,-0.97828925,-0.93706161,
      //         0.90207696,0.000,19,3.9,4.850,0.000,6.073,0.101,0,10,0.00000000,0x0000,172
      //
      // Columns:
      // HEADER,DATE,TIME,PH_INT,PH_EXT,TEMP,TEMP_CTD,S_CTD,O_CTD,P_CTD,VRS_INT,VRS_EXT,V_THERM,V_SUPPLY,I_SUPPLY,HUMIDITY,
      //         V_5V,V_MBATT,V_ISO,V_ISOBATT,I_B,I_K,V_K,STATUS,CHECKSUM,TERMINATOR
      // Note: columns are comma seperated, missing values are nan strings
      //  AS = ASCII string, AF = ASCII float, AI = ASCII integer
      //   HEADER: "SAT" followed by 3 characters identifying frame type (PHL = pH Long), followed by serial number (4 characters): AS 10
      //   DATE: Sample Date (UTC), YYYYDDD: AI 7
      //   TIME: Sample Time (UTC), DECIMALHOUR: AF 9-10
      //   PH_INT: internal calculated pH in total scale: AF 7-8
      //   PH_EXT: external calculated pH in total scale: AF 7-8
      //   TEMP: ISFET Thermister temperature (degrees C): AF 6-8
      //   TEMP_CTD: CTD temperature (degrees C): AF 6-8
      //   S_CTD: CTD salinity (psu): AF 6-7
      //   O_CTD: CTD oxygen concentration (ml/L): AF 5-6
      //   P_CTD: CTD pressure (dbar): AF 5-6
      //   VRS_INT: internal FET voltage (volts): AF 10-11
      //   VRS_EXT: external FET voltage (volts): AF 10-11
      //   V_THERM: thermister voltage (volts): AF 10
      //   V_SUPPLY: supply voltage (volts): AF 5-6
      //   I_SUPPLY: supply current (mA): AI ???
      //   HUMIDITY: enclosure relative humidity (%): AF 3-4
      //   V_5V: Internal 5V supply voltage (volts): AF 5
      //   V_MBATT: Main battery pack voltage (volts): AF 5-6
      //   V_ISO: Internal isolated supply voltage (volts): AF 5
      //   V_ISOBATT: Isolated battery pack voltage (volts): AF 5
      //   I_B: Substrate leakage current (nA): AI ???
      //   I_K: Counter electrode leakage current (nA): AI ???
      //   V_K: Counter electrode voltage (volts): AF 10-11
      //   STATUS: Status word (16-bit hex formateed bitmask): AS 6
      //   CHECKSUM: checksum value: AI 1-3
      //   TERMINATOR: <CR><LF>: AS 2

      // Flush and resync ourselves on a line
      term_flush( auxPort );
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 2: time to wake up SeaFET!");
      // Wake up SeaFET
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 2.1!");
      serialPutLine( auxPort, "!!!!!" );
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 2.2!");
      // Wait 1 second
      sleep( 2 );
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 2.3!");
      // Send sampling command "s"
      
     if ( serialChat( auxPort, "s", "s", 2000L, "s" ) < 1 )
        {
          LOGPRINT( LVL_WARN, "SEAFET: Sensor failed to echo 's' after "           
                   "sending sample 's' command ( one attempt made )." );
        } 
        else 
        {
          //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 3: read samples from buffer!");

          // save full data frame to file
          while ( ( bytesRead = serialGetLine( auxPort, buffer, AUXBUFFLEN, 8000L, "\n" ) ) > 0 ) 
            {
              fwrite( buffer, 1, bytesRead, fpAuxiliary );
            } 
            term_flush( auxPort );
            fflush(fpAuxiliary);
        } 
        //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint 4: time to reset sample loop!");

        timeLastSampled = nowTimeT;

    } // if ( ( nowTimeT - timeLastSampled ) > (opts.auxiliarySamplePeriod * 60) )
    else
    {
      //LOGPRINT( LVL_EMRG, "wsSEAFET(): Checkpoint A: sleeping for 10 seconds...");
      sleep( 10 );
    }
    fflush(stderr);
    fflush(stdout);

  } // for(;;)...

  // Should never ever get here. But...
  cleanup( FAILURE );

} //wsSEAFET()..



int initialize() {
  static char defaultConfigFile[] = "/usr/local/orcaD/orcad.cfg";

  // Initialize ourselves
  opts.debugLevel = 4;
  opts.isDaemon = 1;
  opts.auxiliarySamplePeriod = 15;
  opts.auxiliaryArchiveDownloadPeriod = 1440;
  opts.auxiliaryDataPrefix = "AUX";
  opts.configFileName = defaultConfigFile;
  opts.dataFilePrefix = '\0';
  opts.dataDirName[0] = '\0';
  opts.dataSubDirName[0] = '\0';
  opts.serialPorts = '\0';
  opts.missions = '\0';

  return( SUCCESS );
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
        //cleanup( FAILURE );
        exit( FAILURE );
      case 'f':
                opts.isDaemon = 0;
                break;
      default: usage(); break;
    }
  }
}


void cleanup (int passed_signal)
{   
  time_t nowTimeT;
  struct tm *nowTM;
  char dataLogFile[FILEPATHMAX];
  sigset_t block;
  sigset_t oblock;

  // Block all signals 
  (void) sigfillset (&block);
  sigprocmask (SIG_SETMASK, &block, &oblock);

  // Try and close the auxiliary file
  fclose( fpAuxiliary );

  // Move the temporary file into a new AUX file
  updateDataDir();
                  
  time( &nowTimeT );  
  nowTM = localtime( &nowTimeT );
  sprintf( dataLogFile,"%s/%s%04d%02d%02d%02d%02d.AUX", opts.dataSubDirName,
           opts.auxiliaryDataPrefix,
           nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
           nowTM->tm_hour, nowTM->tm_min );
  LOGPRINT( LVL_ALRT, "Saving data to %s.", dataLogFile );
  rename( "/usr/local/orcaD/data/tmpAuxFile", dataLogFile );

  // Say our last goodbye
  LOGPRINT( LVL_ALRT, "cleanup(): Even though extra data bits are spilling "
                      "all over the floor we are closing shop.  Goodbye and "
                      "thanks for all the fish." );

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

