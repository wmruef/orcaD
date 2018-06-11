/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * weatherd.c : The weather logging daemon  
 *
 * Created: March 2010
 * Updated: November 2015 - wind averaging
 *          August 2016   - added Soundnine/Vaisala compass/anemometer driver
 *
 * Authors: Wendi Ruef <wruef@ocean.washington.edu>
 *          Robert Hubley <rhubley@gmail.com>
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
#include <math.h>

#include <ftdi.h>
#include <weatherd.h>
#include <hardio.h>
#include <buoy.h>
#include <log.h>
#include <util.h>
#include <general.h>
#include <orcad.h>
#include <parser.h>
#include <serial.h>

#define FAILURE -1
#define LCKFILE "/var/run/weatherd.pid"
#define Name "weatherd"
#define LOGFILE "/usr/local/orcaD/logs/weatherd"
#define STDERR stderr
#define METBUFFLEN 256

extern const char *Version;
struct ftdi_context *ftdic = NULL;
FILE * fpWeather = NULL;
FILE * fpInstWeather = NULL;
time_t timeLastArchiveCreated;
struct s_CombinedWeatherData wData;


/*
 * weatherd:
 *
 * A deamon for reading a data stream or to poll a 
 * set of weather sensors.  
 *
 *  Current supported devices:
 * 
 *    - Gill Metpak + R.M. Young Anemometer + Licor
 *
 *    - AirMAR + Licor [DEPRECATED -- see version 
 *                      0.8.9 or earlier for code to 
 *                      run these ] 
 *
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
  //int weatherDeviceType = -1;
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

  if ( ( fpWeather = fopen("/usr/local/orcaD/data/tmpMetFile", "w" ) ) == NULL )
  {     
    LOGPRINT( LVL_EMRG, "Could not open up a new weather file!");
    cleanup( FAILURE );
  }

  if ( hasSerialDevice( GILLMETPAK ) > 0 ||
       hasSerialDevice( RMYOUNGWIND ) > 0 ||
       hasSerialDevice( S9VAISALA ) > 0 )
  {
    wsMonitor();
  }else
  {
    LOGPRINT( LVL_EMRG, "No weather devices found! Exiting.");
  }

  return( 1 );

} // main();



/*************************************************************************/



int wsMonitor( )
{
  int metPort = -1;
  int windPort = -1;
  // Initialize the time
  time_t nowTimeT;
  struct tm *nowTM;
  tzset();
  time( &nowTimeT );  
  nowTM = localtime( &nowTimeT );
  char nowStr[80];
  char weatherStatFile[FILEPATHMAX];
  char dataLogFile[FILEPATHMAX];
  char buffer[METBUFFLEN];
  int instWeatherCounter = 0;
  float srad = 0;
  float sradTotal = 0.0;
  int sradCount = 0;
  float windSpeedTotal = -1;
  float maxWindSpeed = -1;
  float maxWindSpeedDirection = -1;  
  float windDirectionTrueTotal = -1;
  int numScanned = 0;
  int dummyInt;   // Used as dummy fields in scanf
  int i;
  char *token = NULL;
  int instWindSpeed = 0;
  int instWindDirectionTrue = 0;
  int instWindDirectionCenterline = 0;
  int compassDir = 0;
  int windCompass = 0;
  float PARumolPerMeterSquared = 0.0;
  double math_degT, rad, u_mean, v_mean, NEWrad, NEW_degT;
  double NEWmag = 0;
  float windVectorUTotal = -1;
  float windVectorVTotal = -1;
  char *fileHeader = NULL;
 
  if ( hasSerialDevice( GILLMETPAK  ) > 0 )
  {
     LOGPRINT( LVL_DEBG, "wsMonitor(): Have a GILLMETPAK!");  
    static char *metDesc = "# GILL Metpak Pro\n";
    //int f=strlen(fileHeader);
    //int g=strlen(metDesc);
    fileHeader = realloc( fileHeader,80 * sizeof(char));
    if ( fileHeader == NULL )
    {
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not allocate memory for metDesc!");
      cleanup( FAILURE );
    }
    strcat( fileHeader, metDesc );

    metPort = getDeviceFileDescriptor( GILLMETPAK );
    if ( ! metPort )
    {
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not open up metpak device!");
      cleanup( FAILURE );
    }
  }

  if ( hasSerialDevice( RMYOUNGWIND ) > 0 )
  {
    LOGPRINT( LVL_DEBG, "wsMonitor(): Have a wind sensor RMYOUNGWIND!");
    static char *windDesc = "# R.M Young Anemometer 85106, Compass 32500\n";
    fileHeader = realloc( fileHeader,(strlen(fileHeader)+strlen(windDesc) + 1)*sizeof(char) );
    if ( fileHeader == NULL )
    {
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not allocate memory for windDesc!");
      cleanup( FAILURE );
    }
    strcat( fileHeader, windDesc );

    windPort = getDeviceFileDescriptor( RMYOUNGWIND );
    if ( ! windPort )
    {
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not open up wind device!");
      cleanup( FAILURE );
    }
  }

  if ( hasSerialDevice( S9VAISALA ) > 0 )
  {
    LOGPRINT( LVL_DEBG, "wsMonitor(): Have a wind sensor S9VAISALA!");
    static char *windDesc = "# Soundnine Inline Compass and Accelerometer, Vaisala Ultrasonic Anemometer WMT700 \n";
    fileHeader = realloc( fileHeader,(strlen(fileHeader)+strlen(windDesc) + 1)*sizeof(char) );
    if ( fileHeader == NULL )
    {                  
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not allocate memory for windDesc!");
      cleanup( FAILURE );
    }               
    strcat( fileHeader, windDesc );

    windPort = getDeviceFileDescriptor( S9VAISALA );
    if ( ! windPort )
    {
      LOGPRINT( LVL_EMRG, "wsMonitor(): Could not open up wind device!");
      cleanup( FAILURE );
    }
  }


  // TODO: Don't just assume we have a licor attched.
  static char *solarDesc = "#  Licor-190 PAR A/D Solar Radiation Detector\n";
  fileHeader = realloc( fileHeader,(strlen(fileHeader)+strlen(solarDesc) + 1)*sizeof(char) );
  if ( fileHeader == NULL )
  {
    LOGPRINT( LVL_EMRG, "wsMonitor(): Could not allocate memory for solarDesc!");
    cleanup( FAILURE );
  }
  strcat( fileHeader, solarDesc );

  static char *fieldDesc = 
        "# fields: month, day, year, hour, minute, "
        "barometer, humidity, temperature, dewPoint, "
        "instWindDirectionCenterline, "
        "instWindSpeed, N/A, N/A "
        "instPAR, avgPAR, N/A, N/A, compassDir, instWindDirectionCompass\n";
  fileHeader = realloc( fileHeader,(strlen(fileHeader)+strlen(fieldDesc) + 1)*sizeof(char) );
  if ( fileHeader == NULL )
  {
    LOGPRINT( LVL_EMRG, "wsMonitor(): Could not allocate memory for fieldDesc!");
    cleanup( FAILURE );
  }
  strcat( fileHeader, fieldDesc );

  // Write header to file
  fprintf( fpWeather,"%s", fileHeader);

  // Last minute initializations
  time( &nowTimeT );  
  timeLastArchiveCreated = nowTimeT;
  nowTM = localtime( &nowTimeT );
  nowStr[0] = '\0';

  clearCombinedWeatherData( &wData );

  LOGPRINT( LVL_ALRT, "Logging started" );

  //
  // Collection Loop
  //
  for (;;)
  {
    // Record the time
    time( &nowTimeT );  
    nowTM = localtime( &nowTimeT );

    // Check to see if we need to create a MET archive
    if ( ( nowTimeT - timeLastArchiveCreated ) > (opts.weatherArchiveDownloadPeriod * 60) )
    {
      // Time to close the MET file and reopen a new one
      fclose( fpWeather );

      updateDataDir();
      
      // Move the temporary file into a new MET file
      sprintf( dataLogFile,"%s/%s%04d%02d%02d%02d%02d.MET", 
           opts.dataSubDirName,
           opts.weatherDataPrefix,
           nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
           nowTM->tm_hour, nowTM->tm_min );

      LOGPRINT( LVL_ALRT, "Creating %s.", dataLogFile );
      rename( "/usr/local/orcaD/data/tmpMetFile", dataLogFile );

      if ( ( fpWeather = fopen("/usr/local/orcaD/data/tmpMetFile", "w" ) ) 
            == NULL )
      {     
        LOGPRINT( LVL_ALRT, "Could not open up a new weather file!");
        cleanup( FAILURE );
      }
      fprintf( fpWeather,"%s", fileHeader);
      fflush( fpWeather );

      timeLastArchiveCreated = nowTimeT;
    }

    // TODO Rotate large log files
    
    //
    // Read solar radiation:
    srad = GET_SOLAR_RADIATION_VOLTAGE;
    // Convert [ Grayson Board ]
    // WHERE:
    // opts.solarCalibrationConstant is:
    //   Calibration constant retrieved for our first Licor sensor
    //   from their website:
    //   http://envsupport.licor.com/inc/env/calPDF.jsp?doc=Q40070.pdf
    //
    //   The units are: microamps per 1000 micromol s^-1 m^-2
    //
    // opts.solarMillivoltResistance is: 
    //   The value of the resistor that our electronics department installed
    //   between the sensor and the A/D converter.
    //
    // opts.solarADMultiplier is:
    //   The characteristics of the amplifier circuit designed by our
    //   electronics folks: Volts / millivolt
    //
    // Convert [ Licor 2420 Light Sensor Amplifier ]
    //
    //   PAR (umol m-2 s-1) = ( 1 / (gain * (calConstant/1000)) ) * voltage
    //
    //   Where gain: Hard set on all buoys to 0.150
    //      
    //
    // NOTE: originally the final variable for solar radiation was called 
    // "solarWattsPerMeterSquared" because the original Davis weather station
    // output total solar radiaiton in watts/m2.  The airmar and Gill/RMyoung 
    // stations do not have built in solar sensors, so we use the LICOR LI-190
    // Quantum sensor, which measures PAR (only wavelengths between 400-700 nm).
    // PAR is reported in umol m-2 s-1.  To eliminate confusion, the variable
    // name was changed to "PARumolPerMeterSquared" on 18 November 2015.
    // Also, on 17 November we updated to PAR equation for the LICOR 2420
    // and converted from umol to watts...this was changed on 18 November
    // to eliminate the conversion and report in umol m-2 s-1.

    if ( opts.solarMillivoltResistance == 0 )
    {
      // Assuming Licor 2420 Amplifier
      PARumolPerMeterSquared =  
             srad / ( 0.150 * (opts.solarCalibrationConstant/1000) );
    }else
    {
      // NOTE: originally this equation was as follows:	
      // Create the calibrationMultipler:
      // The units are: micromol s^-1 m^-2 per millivolt
      //float solarCalibrationMultiplier = 1.0 / 
      //         ( opts.solarCalibrationConstant * 
      //           ( 1.0 / 1000000.0 ) * 
      //           opts.solarMillivoltResistance ); 
      //
      //PARumolPerMeterSquared = 
      //      (((srad / opts.solarADMultiplier) * solarCalibrationMultiplier ) / 2) 
      //         * 1000;
      // However, upon re-deriving the equation in November 2015, we were unable to 
      // account for the division of 2 in the equation originally supplied by the 
      // electronics shop.  The equation was simplified and revised below, simply
      // PAR (umol / m-2 s-1) = (voltage * 10^9) / (gain * resistance * calConstant),
      // where 10^9 converts units from Amps to uAmps, and 1000 umol m-2 s-1 to 
      // a final unit of umol m-2 s-1.

      PARumolPerMeterSquared =
             (srad * 1000000000) / ( opts.solarADMultiplier * 
	       opts.solarMillivoltResistance *
	       opts.solarCalibrationConstant) ; 
    }

    sradTotal += PARumolPerMeterSquared;
    sradCount++;

    wData.numSamples++;

    if ( wData.numSamples == opts.samplesBeforeMETUpdate )
    {
      // Read from the met/wind device(s)
      //  -- metPort/windPort

      // METPAK Data
      //
      // Example: <STX>Q,,,1010.3,036.0,+027.6,+011.1,,+9998.0007,+9998.0005,0000.000,+12.0,0B,<EXT>11<CRLF>      //
      //
      // Columns:
      // NODE,DIR,SPEED,PRESS,RH,TEMP,DEWPOINT,PRT,AN1,AN2,DIG1,VOLT,STATUS,CHECK
      // Note: columns are comma separated, missing values are empty fields between commas
      //   <STX>: start of string (ASCII value 2)
      //   Node PollAddress: Q by default
      //   Wind Direction: degrees, valid only if anemometer is installed
      //   WindSpeed   : meters/second, valid only if anemometer is installed
      //   Barometric Pressure: hectoPascals
      //   Relative Humidity: percent
      //   Air Temperature: degrees Celcius
      //   Dewpoint: degrees Celcius
      //   Platinum Resistive Temperature: valid only if PRT is installed
      //   Analog voltage 1: auxiliary analog channel
      //   Analog voltage 2: auxiliary analog channel
      //   Digital signal 1: auxiliary digital channel
      //   Source voltage: volts
      //   Status: 
      //   Checksum:

      // Flush and resync ourselves on a line
      term_flush( metPort );
      serialPutLine( metPort, "?Q" );
      // TODO: Figure out what this really is
      if ( serialGetLine( metPort, buffer, METBUFFLEN, 6000L, "\n" ) < 30 )
      {
        //LOGPRINT( LVL_EMRG, "GILLMETPAK buffer 1: %s", buffer );
        if ( serialGetLine( metPort, buffer, METBUFFLEN, 1000L, "\n" ) < 30 )
        {
          //LOGPRINT( LVL_EMRG, "GILLMETPAK buffer 2: %s", buffer );
          LOGPRINT( LVL_WARN, "Failed receiving after "
                   "?Q command ( two attempts made )." );
        }
      }
      
      // parse buffer
      // Save : barometer ( convert from hectoPascals to inches of mercury )
      //      rel humdity, air temp, dewpoint
      // 
      // old conversion:  1 hectoPascal = 0.0296133971008484 inches of mercury
      // According to the NIST Special Pub. 811 
      //	(http://physics.nist.gov/cuu/pdf/sp811.pdf)
      // 1 inhg = 3.38638 kilo Pascals = 33.8638 hectoPascals @ 0 deg C.
      // 1 hectoPascal = 0.0295300586466965 inhg
      // NOTE: using one scanf doesn't work with a CSV line with optional
      //       values.  I.e "###,###,,####" would through off the scanf
      //       pointers and it would terminate early.  Use strtok instead.
      // <STX>Q
      // Empty (<STX>Q)
      token = strtok(buffer,",");
      //// Empty ( optional WIND DIR )
      ////token = strtok(NULL,",");
      ////LOGPRINT( LVL_EMRG, "GILLMETPAK token 2: %s", token);
      //// Empty ( optional WIND SPEED )
      ////token = strtok(NULL,",");
      ////LOGPRINT( LVL_EMRG, "GILLMETPAK token 3: %s", token);
      // Barometric Pressure
      token = strtok(NULL,",");
      numScanned = sscanf(token,"%f", &(wData.inchesBarometricPressure));
      if ( numScanned == 1 )
        // convert from hectoPascals to inches of mercury
        wData.inchesBarometricPressure *= 0.0295300586466965;
      else
        LOGPRINT( LVL_WARN, "Failed to parse barometic pressure from "
               "metpak!" );
      // Relative Humidity
      token = strtok(NULL,",");
      numScanned = sscanf(token,"%f", &(wData.pctHumidity));
      if (numScanned != 1 )
        LOGPRINT( LVL_WARN, "Failed to parse relative humidity from "
               "metpak!" );
      // Temperature
      token = strtok(NULL,",");
      numScanned = sscanf(token,"%f", &(wData.temperatureCelsius));
      if (numScanned != 1 )
        LOGPRINT( LVL_WARN, "Failed to parse temperature from "
               "metpak!" );
      // Dewpoint
      token = strtok(NULL,",");
      numScanned = sscanf(token,"%f", &(wData.dewpointCelsius));
      if (numScanned != 1 )
        LOGPRINT( LVL_WARN, "Failed to parse dewpoint from "
               "metpak!" );

      // Indicate 
      windCompass = 0;
      if ( hasSerialDevice( RMYOUNGWIND ) > 0 )
      {

        // WIND/COMPASS Data
        //
        // Example: A 0000 3470 0000 0000 0007 0008 3520 3550
        //
        //
        // Columns:
        //   PollAddress : Address of the 32500 RM Young compass unit
        //   WindSpeed   : Wind speed from the 85106 RM Young Anemometer in RAW counts.
        //                 NOTE: This needs to be converted to units using the
        //                       following conversion factors.  Note...the wrong conversions
        //			were used originally, from an outdated manual, specifically we used
        //			the conversion 0.09526 for counts to knots instead of 0.1943.  Old 
        // 			conversions are listed below for reference purposes, with new conversions
        //			listed below.
        //                         m/s: 0.04903 (old)
        //                         mph: 0.1097 (old)
        //                         knots: 0.09526 (old)
        //                         km/hr: 0.1765 (old)
        //			correct conversions (from manual 35200-90S)
        // 			m/s: 0.1000 (new)
        //			mph: 0.2237 (new)
        //			knots: 0.1943 (new)
        //			km/hr: 0.3600 (new)
        //   CorrectedWindDirection*10 : Wind direction from anemomter corrected using the
        //                               electronic compass, multplied by 10.
        //   VIN1         : Unused voltage input.
        //   VIN2         : Unused voltage input.
        //   VIN3         : Voltage from anemomter V1.
        //   VIN4         : Voltage form anemomter V2.
        //   CompassDirection*10 :  Compass direction in degrees, multplied by 10.
        //   UncorrectedWindDirection : Wind direction relative to centerline in degrees, multiplied by 10.
        term_flush( windPort );
        serialPutLine( windPort, "MA!" );
        // TODO: Figure out what this really is
        if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 10 )
          if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 10 )
             LOGPRINT( LVL_WARN, "Failed receiving after "
                "MA! command ( two attempts made )." );


        // parse buffer and save into wData
        instWindSpeed = 0;
        instWindDirectionTrue = 0;
        instWindDirectionCenterline = 0;
        compassDir = 0;
        LOGPRINT( LVL_DEBG, "buffer line: %s", buffer);
        numScanned = sscanf(buffer,
                          "A %4d %4d %4d %4d %4d %4d %4d %4d ", 
          &instWindSpeed, &instWindDirectionTrue,
          &dummyInt, &dummyInt, &dummyInt, &dummyInt, &compassDir,
          &instWindDirectionCenterline );

        // convert from raw counts to knots
        wData.instWindSpeed = (float)instWindSpeed * 0.1943;
	 LOGPRINT( LVL_DEBG, "wind speed: %f vs %d", wData.instWindSpeed, instWindSpeed);

        // convert true wind direction to degrees by dividing by 10.  
	wData.instWindDirectionTrue = ((float)instWindDirectionTrue / 10);
	 LOGPRINT( LVL_DEBG, "wind direction: %f vs %d", wData.instWindDirectionTrue, instWindDirectionTrue);

        // convert compass reading to degrees by dividing by 10
        wData.compassDir = (float)compassDir / 10;
	 LOGPRINT( LVL_DEBG, "compass: %f vs %d", wData.compassDir, compassDir);

        // convert centerline wind direction to degrees by dividing by 10
        wData.instWindDirectionCenterline =  (float)instWindDirectionCenterline / 10;
	 LOGPRINT( LVL_DEBG, "wind direction centerline: %f vs %d", wData.instWindDirectionCenterline, instWindDirectionCenterline);

        windCompass = 1;
      } // END RMYOUNGWIND LOOP
      
      if ( hasSerialDevice( S9VAISALA ) > 0 )
      {
        // WIND/COMPASS Data
        //
        // Example: 
	//     <creport v='1' t='11'>
	//     <S9CD v='1'>185.10,59.65,502,6.93,-86.19,86.20,17.72,1,3</S9CD>
	//     <S9CRD v='1'>-303.33, -398.67, 50.00, -127.67, -1042.67, 69.67, -36.00,  3</S9CRD>
	//     <PORT1>
       	//            $999.00,999.00,999.00,999.00,00.00,99.90,0,0,0,0,13.0
	//     </PORT1>
	//     </creport>
        //
        //  S9CD = processed data from Soundnine compass module
	//	<S9CD v='1'>185.10,59.65,502,6.93,-86.19,86.20,17.72,1,3</S9CD>
	//	PSI, INC, MAG, THETA, PHI, TILT, TEMP, RATE, NS
	// 	        PSI: compass heading (rotation around z axis), degrees
	//		     *** saved as wData.CompassDir
	// 		INC: Angle of the magnetic vector from the XY plane, degrees
	//		MAG: Magnitude of the magnetic vector, gauss
	//		THETA: Rotation around y axis, degrees
	//		PHI: Rotation around the x axis, degrees
	//		TILT: Angle of the acceleration vector from the XY plane
	//		TEMP: Approximate sensor temperature, degrees C
	//		RATE: Sensor sampling rate, Hz
	//		NS: Number os samples in current sample period.  The number of seconds
	//		    in the sample period = NS/RATE
	//  S9CRD = raw data from Soundnine compass module
	//	 <S9CRD v='1'>-303.33, -398.67, 50.00, -127.67, -1042.67, 69.67, -36.00,  3</S9CRD>
	//		MX: Raw magnetometer x value, gauss
	//		MY: Raw magnetometer y value, gauss
	//		MZ: Raw magnetometer z value, gauss
	//		AX: Raw accelerometer x value, (g)
	//		AY: Raw accelerometer y value, (g)
	//		AZ: Raw accelerometer z value, (g)
	//		TEMP: Approximate sensor temperature, degrees C
	//		NS: Number os samples in current sample period.  The number of seconds
	//		    in the sample period = NS/RATE
	//		
	//  PORT1 = data from Vaisala anemomoeter
	//	<PORT1> 
	//	       $999.00,999.00,999.00,999.00,00.00,99.90,0,0,0,0,13.0
	//	</PORT1>
	//		WS: Wind speed, average, m/s
	//		    *** saved as wData.instWindSpeed
	//		WM: Wind speed, minimum, m/s
	//		WP: Wind speed, maximum, m/s
	//		WD: Wind direction, average, degrees
	//		    *** saved as wData.instWindDirectionCenterline
	//		DM: Wind direction minimum, average, degrees
	//		DX: Wind direction maximum, average, degrees
	//		GU: Wind gust speed, m/s
	//		TS: Sonic temperature, degrees C
	//		VA: Validity of the measurement data
	//			0 = Unable to measure
	//			1 = Valid wind measurement data
	//		FS: Error codes:
	//			0 = no error
	//			1 = Wind speed exceeds operating limits
	//			2 = Sonic temperature exceed operating limits
	//			3 = Wind speed and sonic temp exceed operating limits
	//		VI: Supply voltage, volts		
	//	
        // 
        //  
      	// Read buffer from S9
        term_flush( windPort );

        // 
        // Theory: Each time we loop through here we parse 1 sentence from the
        //         serial port.  2/3rds of the time we ignore the sentence.
	// 	   We cycle through 6 times to read an entire record.
	//
	for ( i = 0; i < 6; i++ ) {
	  LOGPRINT( LVL_DEBG, "mark 1: iteration: %d", i);
          if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 5 )       
            if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 5 )
               LOGPRINT( LVL_WARN, "Failed receiving data "
                  "from S9 device ( two attempts made )." );
	       LOGPRINT( LVL_DEBG, "weatherd: Entering S9 loop, ready to read buffer.");
	       int stype = S9GetSentenceType ( buffer );
	       LOGPRINT( LVL_DEBG, "mark 2: iteration: %d", i);
	       LOGPRINT( LVL_DEBG, "buffer line: %s", buffer);
	       switch( stype )
	       {
	         case CREPORT:
	           // ignore this data line for now
                  LOGPRINT( LVL_DEBG, "weatherd: Read CREPORT, ignoring." );
		  LOGPRINT( LVL_DEBG, "mark 3: iteration: %d", i);
                  break;

	         case S9CD:
	         LOGPRINT( LVL_DEBG, "weatherd: Read S9CD, processing..." );
		 LOGPRINT (LVL_DEBG, "buffer line: %s", buffer);
	         numScanned = sscanf(buffer, "<S9CD v='1'>%f,%f,%d,%f,%f,%f,%f,%d,%d</S9CD>",
		    &(wData.compassDir), &(wData.INC), &(wData.MAG), &(wData.THETA),
	    	    &(wData.PHI), &(wData.TILT), &(wData.TEMP), &(wData.RATE), &(wData.NS) );
		 LOGPRINT (LVL_DEBG, "Compass: %f", wData.compassDir);
		 LOGPRINT( LVL_DEBG, "mark 4: iteration: %d", i);
	         break;

	         case S9CRD:
	          // ignore this data line for now
	          LOGPRINT( LVL_DEBG, "weatherd: Read S9CRD, ignoring." );
		  LOGPRINT( LVL_DEBG, "mark 5: iteration: %d", i);
	          break;

	         case PORT1:
	 	   LOGPRINT( LVL_DEBG, "weatherd: Read PORT1, processing...");
	            if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 10 )
	              if ( serialGetLine( windPort, buffer, METBUFFLEN, 1000L, "\n" ) < 10 )
           	        LOGPRINT( LVL_WARN, "Failed receiving data after <PORT1> tag ( two attempts made )." );
			LOGPRINT (LVL_DEBG, "buffer line: %s", buffer);
                        numScanned = sscanf(buffer, "$%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f",
                           &(wData.instWindSpeed), &(wData.WM), &(wData.WP), &(wData.instWindDirectionCenterline),
                           &(wData.DM), &(wData.DX), &(wData.GU), &(wData.TS), &(wData.VA), &(wData.FS),
		           &(wData.VI) );
			LOGPRINT (LVL_DEBG, "windSpeed: %f, windDirection: %f",wData.instWindSpeed, wData.instWindDirectionCenterline); 
	 	   LOGPRINT( LVL_DEBG, "mark 6: iteration: %d", i);
	           break;

	         case PORT1END:
		   // ignore this data line for now
                   LOGPRINT( LVL_DEBG, "weatherd: Read PORT1END, ignoring." );
		   LOGPRINT( LVL_DEBG, "mark 7: iteration: %d", i);
                   break;
              
                 case CREPORTEND:
		   // ignore this date line for now
                   LOGPRINT( LVL_DEBG, "weatherd: Read CREPORTEND, ignoring." );
		   LOGPRINT( LVL_DEBG, "mark 8: iteration: %d", i);
                   break;

                 default:                   
                  LOGPRINT( LVL_DEBG, "weatherd: Unknown sentence: %s", buffer );
		  LOGPRINT( LVL_DEBG, "mark 9: iteration: %d", i);
		  wData.instWindSpeed = 999;
		  wData.instWindDirectionCenterline = 999;
                  break;

	       }; // switch(...
	}; // for ( i = 0; i < 6...  

        LOGPRINT( LVL_DEBG, "weatherd: S9 buffer parse complete." );
       if (wData.instWindSpeed < 900 )
       {
         wData.instWindDirectionTrue = wData.instWindDirectionCenterline + wData.compassDir;
	 if ( wData.instWindDirectionTrue < 0 )
	   wData.instWindDirectionTrue = 360 + wData.instWindDirectionTrue;
         if (wData.instWindDirectionTrue > 360 )
	   wData.instWindDirectionTrue = wData.instWindDirectionTrue - 360;
         windCompass = 1;
       }else
       {
	 windCompass = 0;
       }
	

      } // END S9VAISALA LOOP

      if (windCompass == 1 )
      {
        // The compass derived wind direction needs to be adjusted for
        // magnetic declination.  In these parts it's ~16 degrees East.
        // We add the declination to magnetic bearing to get true bearing, 
        // assuming the convention that East is positive and West is negative.
        wData.instWindDirectionTrue = wData.instWindDirectionTrue + opts.compassDeclination;
        // wrap around rather than report negative results
        if ( wData.instWindDirectionTrue < 0 )
          wData.instWindDirectionTrue = 360 + wData.instWindDirectionTrue;
	if ( wData.instWindDirectionTrue > 360 )
	  wData.instWindDirectionTrue = wData.instWindDirectionTrue - 360;
        //
        // vectorize wind speed and direction to allow for averaging
        //
        // convert from weather coordinates to math coordinates
        math_degT = 270 - wData.instWindDirectionTrue;
        if (math_degT < 0 )
	  math_degT += 360;
        // convert from degrees to radians
        rad = math_degT * ((double)3.14159 /(double) 180.0 );
        wData.instU = wData.instWindSpeed * cos(rad);
        wData.instV = wData.instWindSpeed * sin(rad);
      }else
      {
       // Set some rational defaults to indicate problems with instrument or lack of instrument.
         LOGPRINT( LVL_DEBG, "weatherd: No wind sensor deployed; setting variables to default -555." );
         wData.instWindDirectionCenterline = -555;
         wData.instWindSpeed = -555;
         wData.maxWindSpeedDirectionCenterline = -555;
         wData.maxWindSpeed = -555;
         wData.compassDir = -555;
	 wData.instWindDirectionTrue = -555;
      }

      time( &nowTimeT );  
      nowTM = localtime( &nowTimeT );
      nowStr[0] = '\0';
      if ( strftime( nowStr, 80, "%m/%d/%Y %H:%M:%S", nowTM ) 
           <= 0 ) 
      {
        LOGPRINT( LVL_WARN,
              "%s: wsMonitor(): Could not format the time!", Name );
        nowStr[0] = '\0';
      }
      fprintf( fpWeather, "%s\t%.3f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t"
             "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%0.2f\t%0.2f\n",
          nowStr,
          wData.inchesBarometricPressure,
          wData.pctHumidity,
          wData.temperatureCelsius,
          wData.dewpointCelsius,
          wData.instWindDirectionCenterline,
          wData.instWindSpeed,
          wData.maxWindSpeedDirectionCenterline,
          wData.maxWindSpeed,
          PARumolPerMeterSquared,
          sradTotal/sradCount,
          wData.latitude, 
          wData.longitude, wData.compassDir, wData.instWindDirectionTrue );
      fflush(fpWeather);

      // DEBUG
      //printf("%s\twindSpeed=%.2f\twindDirection=%.2f\n", nowStr, wData.instWindSpeed,
      //       wData.instWindDirectionTrue );
      // END DEBUG

      windSpeedTotal += wData.instWindSpeed;
      windDirectionTrueTotal += wData.instWindDirectionTrue;
      windVectorUTotal += wData.instU;
      windVectorVTotal += wData.instV;

      if ( wData.instWindSpeed > maxWindSpeed )
      {
        maxWindSpeed = wData.instWindSpeed;
        maxWindSpeedDirection = wData.instWindDirectionTrue;
        // DEBUG
        //printf("Setting Max: windSpeed=%.2f\twindDirection=%.2f\n", maxWindSpeed, 
        //     maxWindSpeedDirection );
        // END DEBUG
      }
      instWeatherCounter++;

      if ( instWeatherCounter == opts.metUpdatesBeforeInstUpdate )
      {
        // Time to obtain a weather report.  Save the
        // most recent weather data to a status file for
        // download by our customers.
        snprintf( weatherStatFile, FILEPATHMAX, "%s/%s", opts.dataDirName,
                  opts.weatherStatusFilename );
        if( ( fpInstWeather = fopen( weatherStatFile, "w" ) ) != NULL )
        {
          nowStr[0] = '\0';
          if ( strftime( nowStr, 80, "%b %d %Y %H:%M:%S", nowTM ) 
               <= 0 ) 
          {
            LOGPRINT( LVL_WARN,
                      "%s: wsMonitor(): Could not format the time!", Name );
            nowStr[0] = '\0';
          }
 
	// Average wind speed and direction vectors
	u_mean =  windVectorUTotal / (float)instWeatherCounter;
	v_mean =  windVectorUTotal / (float)instWeatherCounter;
        // convert U and V components to radians and magnitude
        NEWmag = sqrt(pow(u_mean,2) + pow(v_mean,2));
	NEWrad = atan2(v_mean,u_mean);
        // convert from radians to degrees
	NEW_degT = ((double)180.0 / (double)3.14159 ) * NEWrad;
        // convert from math coordinates to weather coordinates
	NEW_degT += 180;

          fprintf( fpInstWeather,
           "%s\nbarometer = %.3f inches\nhumidity = %.1f %%\n"
           "temperature = %.1f celsius\ndewPoint = %.1f celsius\n"
           "windDirection = %.1f degrees\nwindSpeed = %.1f knots\n"
           "maxWindDirection = %.1f degrees\nmaxWindSpeed = %.1f knots\n"
           "PAR = %.1f umol/s*meter^2\navgPAR = %.1f umol/s*meter^2\n"
           "compassDir = %0.1f degrees\n",
              nowStr,
              wData.inchesBarometricPressure,
              wData.pctHumidity,
              wData.temperatureCelsius,
              wData.dewpointCelsius,
              // Changed the instWeather wind speed/direction to
              // averages ( of metFile data ):
              //wData.instWindDirectionTrue,
              //wData.instWindSpeed,
              //windDirectionTrueTotal / (float)instWeatherCounter,
              NEW_degT,
              windSpeedTotal / (float)instWeatherCounter,
              maxWindSpeedDirection,
              maxWindSpeed,
              PARumolPerMeterSquared,
              sradTotal / (float)sradCount, wData.compassDir );
          fflush(fpInstWeather);
          fclose( fpInstWeather );
        }
        // DEBUG
        //printf("Printing Max: windSpeed=%.2f\twindDirection=%.2f\n", maxWindSpeed, 
        //     maxWindSpeedDirection );
        // END DEBUG

        instWeatherCounter = 0;

        windSpeedTotal = -1;
        windDirectionTrueTotal = -1;
        maxWindSpeed = -1;
        maxWindSpeedDirection = -1;
      }

      clearCombinedWeatherData( &wData );
      sradTotal = 0.0;
      sradCount = 0;
    } // if ( wData.numSamples ==  ......
    else 
    {
      // Give a chance for other system tasks to run
      // NOTE: This was more important on the bitsyX
      //       series of computers. I am sure this
      //       barely makes a difference on a RaspberryPi
      sleep( opts.minTimeBetweenSamples );
    }

    fflush(stderr);
    fflush(stdout);

  } // for(;;)...

  // Should never ever get here. But...
  cleanup( FAILURE );

} //wsMonitor()..



int initialize() {
  static char defaultConfigFile[] = "/usr/local/orcaD/orcad.cfg";

  // Initialize ourselves
  opts.debugLevel = 4;
  opts.isDaemon = 1;
  opts.minTimeBetweenSamples = 10;   // Seconds between instrument samples 
  opts.samplesBeforeMETUpdate = 6;   // # of samples before a metFile update
  opts.metUpdatesBeforeInstUpdate = 10; // # of metFile updates before instWeather update
  opts.weatherArchiveDownloadPeriod = 1440; // Minutes before metFile is archived
  opts.weatherDataPrefix = "MET";
  opts.weatherStatusFilename = "weather-status.dat";
  opts.solarCalibrationConstant = 0;
  opts.solarMillivoltResistance = 0;
  opts.solarADMultiplier = 0;
  opts.configFileName = defaultConfigFile;
  opts.dataFilePrefix = '\0';
  opts.dataDirName[0] = '\0';
  opts.dataSubDirName[0] = '\0';
  opts.serialPorts = '\0';
  opts.missions = '\0';
  opts.compassDeclination = 0.0;

  return( SUCCESS );
}

void clearCombinedWeatherData ( struct s_CombinedWeatherData * wData )
{
  wData->longitude = 0.0;
  wData->latitude = 0.0;
  wData->inchesBarometricPressure = 0.0;
  wData->pctHumidity = 0.0;
  wData->temperatureCelsius = 0.0;
  wData->dewpointCelsius = 0.0;
  wData->instWindDirectionCenterline = 0.0;
  wData->instWindSpeed = 0.0;
  wData->maxWindSpeedDirectionCenterline = 0.0;
  wData->maxWindSpeed = 0.0;
  wData->numSamples = 0;
  wData->instU = 0;
  wData->instV = 0;
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

int S9GetSentenceType ( const char *buff )              
{
  int i;
  if ( buff )            
  {
    for ( i = 0; i < numSentenceTypes; i++ )
    {
      if ( memcmp(buff, S9SentenceNames[i], 5 ) == 0 )
        return i;
    }
  }
  return FAILURE;
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

  // Try and close the weather file
  fclose( fpWeather );

  // Move the temporary file into a new MET file
  updateDataDir();
                  
  time( &nowTimeT );  
  nowTM = localtime( &nowTimeT );
  sprintf( dataLogFile,"%s/%s%04d%02d%02d%02d%02d.MET", opts.dataSubDirName,
           opts.weatherDataPrefix,
           nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
           nowTM->tm_hour, nowTM->tm_min );
  LOGPRINT( LVL_ALRT, "Saving data to %s.", dataLogFile );
  rename( "/usr/local/orcaD/data/tmpMetFile", dataLogFile );

  // Not really necessary
  //ftdi_usb_close(ftdic);
  //ftdi_deinit(ftdic);
 
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

