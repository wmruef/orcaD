/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * orcactrl.c : The main ORCA management interface
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
 *  TODO: We should prompt the user to save the CTD data
 *        before running monitor pressure, view pressure or view
 *        all or monitor all.  All of these funcitons will overwrite
 *        the data stored in the CTD.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include "general.h"
#include "log.h"
#include "ctd.h"
#include "hydro.h"
#include "aquadopp.h"
#include "meterwheel.h"
#include "weather.h"
#include "hardio.h"
#include "buoy.h"
#include "orcad.h"
#include "profile.h"
#include "serial.h"
#include "parser.h"
#include "util.h"
#include "winch.h"

#define LINEBUFFER 180

#define STDOUT stdout
#define STDERR stderr
extern const char *Version;
#define Name "orcactrl"
#define LOGFILE "/usr/local/orcaD/logs/orcactrl"
#define LCKFILE "/var/run/orca.pid"

struct cmdTokensStruct {
  char **tokens;
  int numTokens;
} cmdTokens;

static struct termios stored_settings;

void reset_keypress(void);
void set_keypress(void);
void usage(void);
void displayOptions(void);
void parseCommand( char *commandEntities[], int entityCount );
int parseCommandLine(int argc, char *argv[] );
int orcactlInitialize();
void cleanup_TERM ();
void cleanup_QUIT ();
void cleanup_INT ();
void cleanup_HUP ();
void cleanup_USR1 ();
void cleanup (int passed_signal);
void tokenizeCommand( char * cmdStr, struct cmdTokensStruct *tokenStruct );
void freeCmdTokens( struct cmdTokensStruct *tokens );


// Global variable
int ignoreHWInitError = 0;


int main(int argc, char *argv[], char *envp[])
{
  int Optind;
  char msg[ LINEBUFFER ];

  // Set the umask
  umask( 0077 );

  // Initially point the logging to stderr
  logFile = stdout;

  // Initialize logging
  if ( startLogging( LOGFILE, "orcactrl", LOG_SCREEN_AND_FILE ) < 1 ) {
    fprintf( STDERR, "%s main(): Warning! Could not start " 
             "logging to file %s\n", Name, LOGFILE );
    logFile = stdout;
  }
 
  // Tell everyone we are doing something
  printf("Orcactrl: Initializing hardware...\n");

  // Initialize our data structures
  if ( orcactlInitialize() < 0 ) 
  {
    LOGPRINT( LVL_CRIT, "Could not initialize our datastructures!" );
    exit( 0 );
  }

  // Read command line
  Optind = parseCommandLine(argc, argv); 

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
    LOGPRINT( LVL_EMRG, "%s: Could not create a lock file! Orcad or "
              "another copy of orcactrl might be running.  Kill the "
              "other copy before running orcactrl!", Name );
    exit( FAILURE );
  }


  /* set signal handlers */
  if ( signal( SIGHUP, cleanup_HUP ) == SIG_ERR )
    printf("Could not install SIGHUP handler!\n");
  if ( signal( SIGINT, cleanup_INT ) == SIG_ERR )
    printf("Could not install SIGINT handler!\n");
  if ( signal( SIGQUIT, cleanup_QUIT ) == SIG_ERR ) 
    printf("Could not install SIGQUIT handler!\n");
  if ( signal( SIGTERM, cleanup_TERM ) == SIG_ERR )
    printf("Could not install SIGTERM handler!\n");
  //(void) signal( SIGPIPE, SIG_IGN );
  //(void) signal( SIGCHLD, SIG_DFL);
  

  // Read configuration file
  if ( parseConfigFile( opts.configFileName ) < 0 )
  {
    LOGPRINT( LVL_CRIT, "Could not parse config file. Exiting..." );
    exit(0);
  }

  if ( initializeHardware() < 0 ) 
  {
    LOGPRINT( LVL_CRIT, "Could not initialize hardware!" );
    if ( ignoreHWInitError )
    {
      LOGPRINT( LVL_CRIT, "WARNING: Proceeding anyway due to -f flag." );
    }else
    {
      LOGPRINT( LVL_CRIT, "Run with -f if you would like to ignore "
                          "this warning and proceed with orcactrl." );
      exit(0);
    }
  }
   
  if( Optind < argc ){
    parseCommand( &(argv[ Optind ]), argc - Optind );
  } else 
  {
    fprintf( STDOUT, "\n\nORCA Control Program Ready\n" );
    while( 1 )
    {
      fprintf( STDOUT, "orcactrl> " );
      if( fgets( msg, sizeof(msg), stdin ) == 0 ) break;
      tokenizeCommand( msg, &cmdTokens );
      if ( cmdTokens.numTokens > 0  &&
           ( strcasecmp( cmdTokens.tokens[0], "exit" ) == 0 ||
             *cmdTokens.tokens[0] == 'q' || 
             *cmdTokens.tokens[0] == 'Q' ) ) 
      {
        printf("Bye Bye...\n");
        break;
      }
      parseCommand( cmdTokens.tokens, cmdTokens.numTokens );
    }
  }
  // TODO:
  //cleanup(0);
  return(0);
}

  
void freeCmdTokens( struct cmdTokensStruct *tokenStruct )
{
  free( tokenStruct->tokens );
}

void tokenizeCommand( char * cmdStr, struct cmdTokensStruct *tokenStruct )
{
  int i, tokenCount, lastCharNull;
  char *tok;
  char *tokLoc;

  // Count tokens
  lastCharNull = 1;
  tokenCount = 0;
  i = 0;
  while ( cmdStr[i] != '\0' ) 
  {
    if ( cmdStr[i] == ' '  || cmdStr[i] == '\t' ||
         cmdStr[i] == '\n' || cmdStr[i] == '\r' )
    {
      lastCharNull = 1;
    }else 
    {
      if ( lastCharNull == 1 ) 
      {
        tokenCount++;
      }
      lastCharNull = 0;
    }
    i++;
  }

  tokenStruct->tokens = (char **)realloc( tokenStruct->tokens, 
                                          sizeof( char * ) * tokenCount );

  i = 0;
  tokLoc = cmdStr;
  while ( ( tok = strsep( &tokLoc, " \t\n\r" ) ) != NULL &&
          i < tokenCount ) 
  {
    if ( tok[0] != '\0' ) 
    {
      tokenStruct->tokens[i++] = tok;
    }
  }
  tokenStruct->numTokens = i;

}



int orcactlInitialize()
{
  static char defaultConfigFile[] = "/usr/local/orcaD/orcad.cfg";

  // Initialize ourselves
  opts.debugLevel = 4;
  opts.minDepth = 2;
  opts.maxDepth = 2;
  opts.parkingDepth = 2;
  opts.isDaemon = 0;
  opts.lastCastNum = 0;
  opts.inCritical = 0;
  opts.weatherArchiveDownloadPeriod = 1440;
  opts.weatherDataPrefix = "MET";
  opts.weatherStatusFilename = "weather-status.dat";
  opts.ctdCalPA1 = 248.24555;
  opts.ctdCalPA2 = -6.524518E-2;
  opts.ctdCalPA3 = 5.430179E-8;
  opts.meterwheelCFactor = 1.718213058;
  opts.configFileName = defaultConfigFile;
  opts.dataFilePrefix = '\0';
  opts.dataDirName[0] = '\0';
  opts.dataSubDirName[0] = '\0';
  opts.serialPorts = '\0';
  opts.missions = '\0';


  return ( 1 );

}


void parseCommand( char *commandEntities[], int entityCount )
{

  int i, ret, tgtDepth, intValue;
  int *depths;
  int hydroFD, hydroType, weatherFD, aquaFD;
  struct sPort *mwPort;
  char key;
  float floatVal1, floatVal2, floatVal3;
  double doubleVal1, doubleVal2;
  struct mission *mptr;
  FILE *outFile;
  //int i = 0;
  //for ( i = 0; i < entityCount; i++ )
  //{
  //  printf("Entity = %s\n", commandEntities[i] );
  //}

  if ( entityCount > 0 ) 
  {

    if ( strcasecmp( "help", commandEntities[0] ) == 0 ||
         strcasecmp( "h", commandEntities[0] ) == 0 ||
         strcasecmp( "?", commandEntities[0] ) == 0 )
    {
      displayOptions();   
    }else if ( strcasecmp( "test2", commandEntities[0] ) == 0 )
    {
      hydroType = getHydroWireDeviceType();
      hydroFD = getDeviceFileDescriptor( hydroType );
      
      if ( serialChat( hydroFD, "OUTPUTFORMAT=3\r",
                        "OUTPUTFORMAT=3\r\n", 1000L, "\r\nS>" ) < 1 )
      {
        sleep(1);
        if ( serialChat( hydroFD, "OUTPUTFORMAT=3\r",
                        "OUTPUTFORMAT=3\r\n", 1000L, "\r\nS>" ) < 1 )
        {
           LOGPRINT( LVL_WARN, "Failed sending "
                "OUTPUTFORMAT=3 command after two attempts." );
        }
      }

    }else if ( strcasecmp( "view", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "config", commandEntities[1] ) == 0 )
        {
          logOpts( stdout );
        }else if ( strcasecmp( "aquafat", commandEntities[1] ) == 0 )
        {
          if ( hasSerialDevice( AQUADOPP ) > 0 )
          {
            aquaFD = getDeviceFileDescriptor( AQUADOPP );
            LOGPRINT( LVL_ALWY, " Running command: view aquafat" );
            if ( displayAquadoppFAT( aquaFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to print the aquadopp config!" );
            }
          }else {
              LOGPRINT( LVL_ALRT, "Sorry...there doesn't seem to be an"
                                  " aquadopp atached.  Check the orcad.cfg"
                                  " file." );
          }
        }else if ( strcasecmp( "aquaconfig", commandEntities[1] ) == 0 )
        {
          if ( hasSerialDevice( AQUADOPP ) > 0 )
          {
            aquaFD = getDeviceFileDescriptor( AQUADOPP );
            if ( displayAquadoppConfig( aquaFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to print the aquadopp config!" );
            }
          }else {
              LOGPRINT( LVL_ALRT, "Sorry...there doesn't seem to be an"
                                  " aquadopp atached.  Check the orcad.cfg"
                                  " file." );
          }
        }else if ( strcasecmp( "voltage", commandEntities[1] ) == 0 ) 
        {
          floatVal1 = GET_INTERNAL_BATTERY_VOLTAGE;
          printf( "Internal Battery Voltage = %6.2f volts\n", floatVal1 );
          floatVal1 = GET_EXTERNAL_BATTERY_VOLTAGE;
          printf( "External Battery Voltage = %6.2f volts\n", floatVal1 );
        }else if ( strcasecmp( "pressure", commandEntities[1] ) == 0 )
        {
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( startHydroLogging( hydroType, hydroFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
          }else 
          {
            doubleVal1 = getHydroPressure( hydroType, hydroFD );
            doubleVal2 = convertDBToDepth( doubleVal1 );
            printf( "Water Pressure = %6.2f db = %6.2f meters\n", 
                    doubleVal1, doubleVal2 );
            if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
            }
          }
        }else if ( strcasecmp( "meterwheel", commandEntities[1] ) == 0 )
        {
          mwPort = getMeterWheelPort();
          floatVal1 = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
          if ( floatVal1 < 0 )
            printf("Meter wheel count could not be read! ( error:  %g )\n", 
                   floatVal1);
          else
            printf("Meter Wheel Count Adjusted = %6.2f meters\n", 
                    floatVal1 );
        }else if ( strcasecmp( "weather", commandEntities[1] ) == 0 )
        {
          weatherFD = getDeviceFileDescriptor( DAVIS_WEATHER_STATION );
          logInstantWeather( weatherFD, stdout );
        }else if ( strcasecmp( "all", commandEntities[1] ) == 0 )
        {
          mwPort = getMeterWheelPort();
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( startHydroLogging( hydroType, hydroFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
          }else 
          {
            floatVal1 = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
            floatVal2 = GET_INTERNAL_BATTERY_VOLTAGE;
            floatVal3 = GET_EXTERNAL_BATTERY_VOLTAGE;
            doubleVal1 = getHydroPressure ( hydroType, hydroFD );
            doubleVal2 = convertDBToDepth( doubleVal1 );
            LOGPRINT( LVL_ALWY, " Running command: view all" );
            LOGPRINT( LVL_ALWY,
                      "pres=%6.2fdb prdp=%6.2fm mwdp=%6.1fm "
                      "ipwr=%4.1fv epwr=%4.1fv", doubleVal1, doubleVal2,
                      floatVal1, floatVal2, floatVal3 );
            if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
            }
          }
        }else{
          printf("Error: Command %s %s unknown!\n", commandEntities[0], 
                 commandEntities[1] );
        }
      }else 
      {
         printf("Error: Command has too many/few paramters!\n" );
      }
    }else if ( strcasecmp( "set", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 3 )
      {
        if ( strcasecmp( "minDepth", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &intValue ) == 1 ) 
          { 
            LOGPRINT( LVL_ALWY, "Changing Parameter: minDepth = %d", 
                      intValue );
            opts.minDepth = intValue;
          }else{
            printf("Don't understand argument %s\n", commandEntities[2] ); 
          }
        }else if ( strcasecmp( "parkingDepth", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &intValue ) == 1 ) 
          { 
            LOGPRINT( LVL_ALWY, "Changing Parameter: parkingDepth = %d", 
                      intValue );
            opts.parkingDepth = intValue;
          }else{
            printf("Don't understand argument %s\n", commandEntities[2] ); 
          }
        }else if ( strcasecmp( "maxDepth", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &intValue ) == 1 ) 
          { 
            LOGPRINT( LVL_ALWY, "Changing Parameter: maxDepth = %d", 
                      intValue );
            opts.maxDepth = intValue;
          }else{
            printf("Don't understand argument %s\n", commandEntities[2] ); 
          }
        }else if ( strcasecmp( "debugLevel", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &intValue ) == 1 ) 
          { 
            LOGPRINT( LVL_ALWY, "Changing Parameter: debugLevel = %d", 
                      intValue );
            opts.debugLevel = intValue;
          }else{
            printf("Don't understand argument %s\n", commandEntities[2] ); 
          }
        }else
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Too many/few arguments!\n");
      }
    }else if ( strcasecmp( "monitor", commandEntities[0] ) == 0 )
    { 
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "voltage", commandEntities[1] ) == 0 ) 
        {
          set_keypress();
          i = 0;
          printf("\nPress 'x' to exit.\n");
          while ( ( key = getchar() ) != 'x' ) 
          {
            if ( i++ == 20 )
            {
              printf("\nPress 'x' to exit.\n");
              i = 0;
            }
            floatVal1 = GET_INTERNAL_BATTERY_VOLTAGE;
            printf( "Internal Battery Voltage = %6.2f volts\n", floatVal1 );
            floatVal1 = GET_EXTERNAL_BATTERY_VOLTAGE;
            printf( "External Battery Voltage = %6.2f volts\n", floatVal1 );
            sleep(1);
          }
          reset_keypress();
        }else if ( strcasecmp( "pressure", commandEntities[1] ) == 0 )
        {
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( startHydroLogging( hydroType, hydroFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
          }else 
          {
            set_keypress();
            i = 0;
            printf("\nPress 'x' to exit.\n");
            while ( ( key = getchar() ) != 'x' ) 
            {
              if ( i++ == 20 )
              {
                printf("\nPress 'x' to exit.\n");
                i = 0;
              }
              doubleVal1 = getHydroPressure( hydroType, hydroFD );
              doubleVal2 = convertDBToDepth( doubleVal1 );
              printf( "Water Pressure = %6.2f db = %6.2f meters\n", 
                      doubleVal1, doubleVal2 );
              sleep(1);
            }
            reset_keypress();
            if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
            }
          }
        }else if ( strcasecmp( "meterwheel", commandEntities[1] ) == 0 )
        {
          mwPort = getMeterWheelPort();
          set_keypress();
          i = 0;
          printf("\nPress 'x' to exit.\n");
          while ( ( key = getchar() ) != 'x' ) 
          {
            if ( i++ == 20 )
            {
              printf("\nPress 'x' to exit.\n");
              i = 0;
            }
            floatVal1 = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
            printf("Meter Wheel Count Adjusted = %6.2f meters\n", 
                    floatVal1 );
            sleep(1);
          }
          reset_keypress();
        }else if ( strcasecmp( "all", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, " Running command: monitor all" );
          mwPort = getMeterWheelPort();
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( startHydroLogging( hydroType, hydroFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
          }else 
          {
            set_keypress();
            i = 0;
            printf("\nPress 'x' to exit.\n");
            while ( ( key = getchar() ) != 'x' ) 
            {
              if ( i++ == 20 )
              {
                printf("\nPress 'x' to exit.\n");
                i = 0;
              }
              floatVal1 = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
              floatVal2 = GET_INTERNAL_BATTERY_VOLTAGE;
              floatVal3 = GET_EXTERNAL_BATTERY_VOLTAGE;
              doubleVal1 = getHydroPressure ( hydroType, hydroFD );
              doubleVal2 = convertDBToDepth( doubleVal1 );
              LOGPRINT( LVL_ALWY,
                        "pres=%6.2fdb prdp=%6.2fm mwdp=%6.1fm "
                        "ipwr=%4.1fv epwr=%4.1fv", doubleVal1, doubleVal2,
                        floatVal1, floatVal2, floatVal3 );
              sleep(1);
            }
            reset_keypress();
            if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
            }
          }
        }else{
          printf("Error: Command %s %s unknown!\n", commandEntities[0], 
                 commandEntities[1] );
        }
      }else 
      {
         printf("Error: Command has too many/few paramters!\n" );
      }
      
    }else if ( strcasecmp( "move", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 3 )
      {
        if ( strcasecmp( "up", commandEntities[1] ) == 0 ) 
        {
          if ( sscanf(commandEntities[2], "%d", &tgtDepth ) == 1 ) 
          {
            LOGPRINT( LVL_ALWY, "Running Command: move up %d", tgtDepth );
            hydroType =  getHydroWireDeviceType();
            hydroFD = getDeviceFileDescriptor( hydroType );
            mwPort = getMeterWheelPort();
            if ( startHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
            }else 
            {
              ret = movePackageUp( hydroFD, hydroType, mwPort, tgtDepth );
              LOGPRINT( LVL_ALWY, "Command Returned: %d", ret );
              if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
              {
                LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
              }
            }
          }else
          {
            printf("Incorrect argument!  Need a number!\n");
          }
        }else if ( strcasecmp( "down", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &tgtDepth ) == 1 ) 
          {
            LOGPRINT( LVL_ALWY, "Running Command: move down %d", tgtDepth );
            hydroType = getHydroWireDeviceType();
            hydroFD = getDeviceFileDescriptor( hydroType );
            mwPort = getMeterWheelPort();
            if ( startHydroLogging( hydroType, hydroFD ) < 0 )
            {
              LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
            }else 
            {
              ret = movePackageDown( hydroFD, hydroType, mwPort, tgtDepth );
              LOGPRINT( LVL_ALWY, "Command Returned: %d", ret );
              if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
              {
                LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
              }
            }   
          }else
          {
            printf("Incorrect argument!  Need a number!\n");
          }
        }else 
        {
          printf("Syntax error!\n");
        }
      }else if ( entityCount > 3 ) {
        if ( strcasecmp( "up-discretely", commandEntities[1] ) == 0 )
        {
          if ( sscanf(commandEntities[2], "%d", &tgtDepth ) == 1 ) 
          {
            //  malloc an int array
            depths = (int *)malloc( sizeof(int) * (entityCount - 3) );
            for ( i = 0; i < (entityCount - 3); i++ )
            {
              if ( sscanf(commandEntities[3+i], "%d", &(depths[i]) ) != 1 ) 
              {
                printf("Error reading depths!\n");
                break;
              }
            }
            if ( i == (entityCount - 3) )
            {
              // TODO: Log discrete depths
              LOGPRINT( LVL_ALWY, "Running Command: move up-discretely"
                        "%d", tgtDepth );
              hydroType = getHydroWireDeviceType();
              hydroFD = getDeviceFileDescriptor( hydroType );
              mwPort = getMeterWheelPort();
              if ( startHydroLogging( hydroType, hydroFD ) < 0 )
              {
                LOGPRINT( LVL_ALRT, "Failed to turn on hydro logging!" );
              }else 
              {
                ret = movePackageUpDiscretely( hydroFD, hydroType, mwPort, 
                                               tgtDepth, depths,
                                               (entityCount-3), 20 );
                LOGPRINT( LVL_ALWY, "Command Returned: %d", ret );
                if ( stopHydroLogging( hydroType, hydroFD ) < 0 )
                {
                  LOGPRINT( LVL_ALRT, "Failed to turn off hydro logging!" );
                }
              }
            }
            if ( depths ) 
              free( depths );
          }else{
            printf("Could not read target depth!\n");
          }
        }else 
        {
          printf("Syntax error!\n");
        }
      }else {
        printf("Error: Command has too many/few paramters!\n" );
      } 
    }else if ( strcasecmp( "profile", commandEntities[0] ) == 0 )
    { 
      if ( entityCount == 2 )
      {
        mptr = opts.missions;
        while ( mptr != NULL ) 
        {
          if ( strcasecmp( mptr->name, commandEntities[1] ) == 0 )
            break;
          mptr = mptr->nextMission;
        }
        if ( mptr != NULL )
        {
          LOGPRINT( LVL_ALWY, "Running Command: profile %s", 
                    commandEntities[1] );
          ret = profile( mptr ); 
          LOGPRINT( LVL_ALRT, "Command Returned: %d", ret );
        }else
        {
          printf("Could not find mission %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Missing mission name!\n");
      }
    }else if ( strcasecmp( "runctd", commandEntities[0] ) == 0 )
    {
      LOGPRINT( LVL_ALWY, "Running command: runctd" );
      hydroType = getHydroWireDeviceType();
      hydroFD = getDeviceFileDescriptor( hydroType );
      startLoggingCTD19Plus( hydroFD );
      set_keypress();
      printf("\nPress 'x' to exit.\n");
      while ( ( key = getchar() ) != 'x' ) 
      {
        sleep(1);
      }
      reset_keypress();
      stopLoggingCTD19Plus( hydroFD );
    }else if ( strcasecmp( "sync", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "ctd", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: sync ctd" );
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( syncHydroTime( hydroType, hydroFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to sync CTD time!" );
          }
        }else if ( strcasecmp( "weather", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: sync weather" );
          weatherFD = getDeviceFileDescriptor( DAVIS_WEATHER_STATION );
          if ( syncWSTime( weatherFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to sync weather station time!" );
          }
        }else if ( strcasecmp( "aquadopp", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: sync aquadopp" );
          aquaFD = getDeviceFileDescriptor( AQUADOPP );
          if ( syncAquadoppTime( aquaFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to sync aquadopp station time!" );
          }
        }else 
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Missing instrument!\n");
      }
    }else if ( strcasecmp( "clear", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "aquadopp", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: clear aquadopp" );
          aquaFD = getDeviceFileDescriptor( AQUADOPP );
          if ( clearAquadoppFAT( aquaFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to clear the aquadopp recorder !" );
          }
        }else 
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Missing instrument type!\n");
      }
    
    }else if ( strcasecmp( "run", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "aquadopp", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: run aquadopp" );
          aquaFD = getDeviceFileDescriptor( AQUADOPP );
          if ( startLoggingAquadopp( aquaFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to start aquadopp logging!" );
          }
          set_keypress();
          printf("\nPress 'x' to exit.\n");
          while ( ( key = getchar() ) != 'x' ) 
          {
            sleep(1);
          }
          reset_keypress();
          if ( stopLoggingAquadopp( aquaFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "Failed to stop aquadopp logging!" );
          }
        }else 
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Missing instrument!\n");
      }
    }else if ( strcasecmp( "test", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 2 )
      {
        if ( strcasecmp( "ctd_comm", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: test ctd_comm" );
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          float dataLoss = testCTDCommunication( hydroType, hydroFD );
          printf( "%6.2f %% data loss\n", dataLoss );
        }else 
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else
      {
        printf("Missing device!\n");
      }
    }else if ( strcasecmp( "download", commandEntities[0] ) == 0 )
    {
      if ( entityCount == 3 )
      {
        if ( strcasecmp( "ctd", commandEntities[1] ) == 0 )
        {
          hydroType = getHydroWireDeviceType();
          hydroFD = getDeviceFileDescriptor( hydroType );
          if ( ( outFile = fopen( commandEntities[2], "w+" ) ) != NULL ) 
          {
            LOGPRINT( LVL_ALWY, "Running Command: download ctd %s",
                      commandEntities[2] );
            ret = downloadHydroData( hydroType, hydroFD, outFile );
            LOGPRINT( LVL_ALRT, "Command Returned: %d", ret );
            fclose( outFile );
          }else {
            LOGPRINT( LVL_ALRT, "Could not open file %s!",
                      commandEntities[2] );
          }
       }else if ( strcasecmp( "weather", commandEntities[1] ) == 0 )
        {
          LOGPRINT( LVL_ALWY, "Running Command: download weather %s",
                    commandEntities[2] );
          weatherFD = getDeviceFileDescriptor( DAVIS_WEATHER_STATION );
          if ( ( outFile = fopen( commandEntities[2], "w+" ) ) != NULL ) 
          {
            ret = downloadWeatherData( weatherFD, outFile, 0 );
            LOGPRINT( LVL_ALRT, "Command Returned: %d", ret );
            fclose( outFile );
          }else {
            LOGPRINT( LVL_ALRT, "Could not open file %s!",
                      commandEntities[2] );
          }
        }else 
        {
          printf("Don't understand %s\n", commandEntities[1] );
        }
      }else if ( entityCount == 2 && 
                 strcasecmp( "aquadopp", commandEntities[1] ) == 0 )
      {
        LOGPRINT( LVL_ALWY, "Running command: download aquadopp" );
        aquaFD = getDeviceFileDescriptor( AQUADOPP );
        if ( downloadAquadoppFiles( aquaFD ) < 0 )
        {
          LOGPRINT( LVL_ALRT, "Failed to download Aquadopp recorder files!" );
        }
      }else
      {
        printf("Missing filename!\n");
      }
    }else 
    {
      printf("Error: Command %s unknown!\n", commandEntities[0] );
    }
  }


}


int parseCommandLine(int argc, char *argv[] )
{
  int option;
  while ((option = getopt (argc, argv, "d:vfhc:" )) != EOF) 
  {
    switch (option) 
    {
      case 'c':
                opts.configFileName =
                      (char * )malloc( (strlen( optarg )+1) * sizeof( char ) );
                strcpy( opts.configFileName, optarg );
                break;
      case 'd': 
                sscanf( optarg, "%d", &opts.debugLevel );
                break;
      case 'f': 
	        ignoreHWInitError = 1;      
                break;
      case 'v':
                fprintf( STDOUT, "%s\n", Version );
                exit(1);
      case 'h':
                usage();
      default:
                usage();
    }
  }
  return ( optind );
}

 char *msg[] ={
 "\nusage: %s [-d debuglevel] [-c config file] [-h] [command]\n\n",
 " The main ORCA control utility.  NOTE: This utility should only be\n",
 " used when the orcaD process has been stopped!\n\n",
 " General options:\n",
 "  -d (debuglevel)  - debug level\n",
 "  -c (config file) - hardware config file\n",
 "  -h               - Print this information\n\n",
 " Commands:\n",
 "  view      config |                          - View orcad.cfg\n",
 "            aquaconfig |                      - .. aquadopp config\n",  
 "            aquafat |                         - .. aquadopp recorder files\n", // TODO
 "            voltage |                         - .. internal/external power\n",
 "            weather |                         - .. Davis weather output\n",
 "            pressure |                        - .. CTD Pressure\n", 
 "            meterwheel |                      - .. Meterwheel count\n",
 "            all                               - .. General buoy state\n",
 "  set       minDepth (meters) |               - Set various parameters.\n",
 "            parkingDepth (meters) |\n",
 "            maxDepth (meters) |\n",
 "            debugLevel (number)\n",
 "  monitor   voltage|pressure|meterwheel|all   - Watch buoy state.\n",
 "  move      up (target meters)                - Move the package up to\n",
 "                                                the specified depth in meters.\n",
 "            down (target meters)              - Move the package down to\n",
 "                                                the specified depth in meters.\n",
 "            up-discretely (target meters)\n",
 "                          (sample1 meters)\n",
 "                          (sample2 meters)\n",
 "                          ...                 - Move the package up discretely.\n",
 "  profile   (mission name)                    - Run through a profile.\n",
 "  download  ctd|weather (filename) |          - Download data to a file.\n",
 "            aquadopp                          - Download aquadopp data to\n", // TODO
 "                                                autogenerated file names.\n",
 "  clear     aquadopp                          - Erase the data stored on the\n", // TODO
 "                                                aquadopp recorder.\n",
 "  run       aquadopp                          - Run the aquadopp instrument.\n",
 "  test      ctd_comm                          - Test the noise level of the\n",
 "                                                hydrowire. NOTE: Must have a\n",
 "                                                CTD attached.\n",
 "  sync      ctd|weather|aquadopp              - Sync instrument times to bitsyX.\n\n",
  0} ;

void displayOptions(void)
{
  int i;
  char *s;
  for( i = 0; (s = msg[i]); ++i ){
    if( i == 0 ){
      fprintf( STDERR, s, Name );
    } else {
      fprintf( STDERR, "%s", s );
    }
  }
}

void usage(void)
{
  displayOptions();
  fprintf( STDOUT, "version = %s\n\n", Version );
  exit(1);
}


void cleanup (int passed_signal)
{
  // block all signals ( or ensure that it's been done for you )

  // Don't use printf!
  printf("Caught! %d\n", passed_signal );

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
  }

  // write to log  (using write?)
  
  // shut off all io 
  
  // close all file descriptors

  //exit( errorcode? );
  exit(1);
}

void cleanup_USR1 ()
{
  // LOGPRINT
  cleanup(SIGUSR1);
}
void cleanup_HUP ()
{
  // LOGPRINT
  cleanup(SIGHUP);
}
void cleanup_INT ()
{
  // LOGPRINT
  cleanup(SIGINT);
}
void cleanup_QUIT ()
{
  // LOGPRINT
  cleanup(SIGQUIT);
}
void cleanup_TERM ()
{
  // LOGPRINT
  cleanup(SIGTERM);
}


void set_keypress(void)
{
    struct termios new_settings;

    tcgetattr(0,&stored_settings);

    new_settings = stored_settings;
  
    /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO);
    new_settings.c_lflag &= (~ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 0;

    tcsetattr(0,TCSANOW,&new_settings);
    return;
}

void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}


