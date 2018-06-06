/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * parser.c :  The config file parser
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include "general.h"
#include "term.h"
#include "log.h"
#include "orcad.h"
#include "parser.h"

// The day names used in config file
static const char *const dayNamesList[] = {
  "Sun",
  "Mon",
  "Tue",
  "Wed",
  "Thu",
  "Fri",
  "Sat",
  NULL
};

// The month names used in the config file
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
//   logOpts - Log the program's settings to a file descriptor
//
// SYNOPSIS
//   #include "general.h"
//   #include "parser.h"
//
//   logOpts( FILE * fd );
//
// DESCRIPTION
//   Log the program's settings in a human readable report
//   format.  This is used by orcad and orcactrl to print
//   the settings to a log file or to the screen.  This
//   function expects an open an writable file descriptor
//   as it's only parameter.
//
// RETURNS
//   Nothing.  Flushes but does not close the passed file
//   descriptor.
//
void logOpts ( FILE * fd ) {
  int i;
  struct sPort *port = NULL;
  struct mission *mptr = NULL;

  fprintf( fd, "GLOBAL PARAMETERS\n" );
  fprintf( fd, "-----------------\n" );
  fprintf( fd, "  isDaemon                        = %d\n", opts.isDaemon );
  fprintf( fd, "  debugLevel                      = %d\n", opts.debugLevel );
  fprintf( fd, "  configFileName                  = %s\n", 
           opts.configFileName );
  fprintf( fd, "  min_depth                       = %d\n", opts.minDepth );
  fprintf( fd, "  max_depth                       = %d\n", opts.maxDepth );
  fprintf( fd, "  parking_depth                   = %d\n", opts.parkingDepth );
  fprintf( fd, "  data_file_prefix                = %s\n", opts.dataFilePrefix);
  fprintf( fd, "  data_storage_dir                = %s\n", opts.dataDirName );
  fprintf( fd, "  last_cast_num                   = %ld\n", opts.lastCastNum );
  fprintf( fd, "  weather_data_prefix             = %s\n", 
           opts.weatherDataPrefix );
  //fprintf( fd, "  weather_sample_period           = %d\n", 
  //         opts.weatherSamplePeriod );
  fprintf( fd, "  weather_archive_download_period = %d\n", 
           opts.weatherArchiveDownloadPeriod );
  fprintf( fd, "  weather_status_filename         = %s\n", 
           opts.weatherStatusFilename );
  //fprintf( fd, "  weather_status_update_period    = %d\n", 
  //         opts.weatherStatusUpdatePeriod );
  fprintf( fd, "  ctd_cal_PA1                     = %le\n", opts.ctdCalPA1 );
  fprintf( fd, "  ctd_cal_PA2                     = %le\n", opts.ctdCalPA2 );
  fprintf( fd, "  ctd_cal_PA3                     = %le\n", opts.ctdCalPA3 );
  if ( opts.solarCalibrationConstant > 0 )
  {
    fprintf( fd, "  solar_calibration_constant      = %le\n", opts.solarCalibrationConstant );
    if ( opts.solarADMultiplier == 0 && opts.solarMillivoltResistance == 0 )
    {
      fprintf(fd, "    INFO: Assuming Licor 2420 Light Sensor Amplifier with 0.150 gain\n");
    }else
    {
      fprintf(fd,  "    INFO: Greyson amplifier board:\n");
      
      fprintf( fd, "       solar_millivolt_resistance      = %le\n", opts.solarMillivoltResistance );
      fprintf( fd, "       solar_ad_multiplier             = %le\n", opts.solarADMultiplier );
    }
  }
  fprintf( fd, "  compass_declination             = %le\n", opts.compassDeclination );
  fprintf( fd, "  meterwheel_cfactor              = %le\n\n", opts.meterwheelCFactor );
  fprintf( fd, "SERIAL DEVICES\n" );
  fprintf( fd, "--------------\n" );
  
  port = opts.serialPorts;
  while ( port != NULL ) {
    if ( port->productID > 0 )
      fprintf( fd, "  USB Port\n");
    else 
      fprintf( fd, "  RS232 Port\n");
    fprintf( fd, "  description   = %s\n", port->description );
    fprintf( fd, "  device_type   = " );
    if ( port->deviceType == SEABIRD_CTD_19 ) 
      fprintf( fd, "SEABIRD_CTD_19\n" );
    if ( port->deviceType == SEABIRD_CTD_19_PLUS ) 
      fprintf( fd, "SEABIRD_CTD_19_PLUS\n" );
    if ( port->deviceType == AGO_METER_WHEEL_COUNTER ) 
      fprintf( fd, "AGO_METER_WHEEL_COUNTER\n" );
    if ( port->deviceType == DAVIS_WEATHER_STATION ) 
      fprintf( fd, "DAVIS_WEATHER_STATION\n" );
    if ( port->deviceType == ENVIROTECH_ECOLAB ) 
      fprintf( fd, "ENVIROTECH_ECOLAB\n" );
    if ( port->deviceType == SATLANTIC_ISIS ) 
      fprintf( fd, "SATLANTIC_ISIS\n" );
    if ( port->deviceType == SATLANTIC_ISIS_X ) 
      fprintf( fd, "SATLANTIC_ISIS_X\n" );
    if ( port->deviceType == CELL_MODEM ) 
      fprintf( fd, "CELL_MODEM\n" );
    if ( port->deviceType == AQUADOPP ) 
      fprintf( fd, "AQUADOPP\n" );
    if ( port->deviceType == AIRMAR_PB100 ) 
      fprintf( fd, "AIRMAR_PB100\n" );
    if ( port->deviceType == AIRMAR_PB200 ) 
      fprintf( fd, "AIRMAR_PB200\n" );
    if ( port->deviceType == GILLMETPAK )
      fprintf( fd, "GILLMETPAK\n" );
    if ( port->deviceType == RMYOUNGWIND )
      fprintf( fd, "RMYOUNGWIND\n" );
    if ( port->deviceType == S9VAISALA )                  
      fprintf( fd, "S9VAISALA\n" ); 
    if ( port->deviceType == SEAFETPH )
      fprintf( fd, "SEAFETPH\n" );
    if ( port->deviceType == ARDUINO_METER_WHEEL_COUNTER ) 
      fprintf( fd, "ARDUINO_METER_WHEEL_COUNTER\n" );
    if ( port->productID > 0 )
    {
      fprintf( fd, "  vendor_id     = %x\n", port->vendorID );
      fprintf( fd, "  product_id    = %x\n", port->productID );
      fprintf( fd, "  serial_id     = %s\n", port->serialID );
      fprintf( fd, "  baud          = %d\n", port->baud );
    }else
    {
      fprintf( fd, "  tty           = %s\n", port->tty );
      fprintf( fd, "  baud          = %d\n", port->baud );
      fprintf( fd, "  stop_bits     = %d\n", port->stopBits );
      fprintf( fd, "  data_bits     = %d\n", port->dataBits );
      if ( port->flow == FC_NONE ) {
        fprintf( fd, "  flow_control  = none\n");
      }else if ( port->flow == FC_RTSCTS ) {
        fprintf( fd, "  flow_control  = cts/rts\n");
      }else {
        fprintf( fd, "  flow_control  = xon/xoff\n");
      }
      if ( port->parity == P_EVEN ) {
        fprintf( fd, "  parity        = even\n");
      }else if ( port->parity == P_ODD ) {
        fprintf( fd, "  parity        = odd\n");
      }else {
        fprintf( fd, "  parity        = none\n");
      }
    }
    fprintf( fd, "\n" );
    port = port->nextPort;
  }

  fprintf( fd, "MISSIONS\n" );
  fprintf( fd, "--------\n" );
  mptr = opts.missions;
  while ( mptr != NULL ) {
    fprintf( fd, "  name                 = %s\n", mptr->name );
    fprintf( fd, "  start_min            = %s\n", 
             convArrayToRangeString( mptr->startMinsList, 60, NULL ) );
    fprintf( fd, "  start_hour           = %s\n", 
             convArrayToRangeString( mptr->startHoursList, 24, NULL ) );
    fprintf( fd, "  start_day_of_month   = %s\n", 
             convArrayToRangeString( mptr->startDaysOfMonthList, 32, NULL ) );
    fprintf( fd, "  start_month          = %s\n", 
             convArrayToRangeString( mptr->startMonthsList, 12, 
                                     monthNamesList ) );
    fprintf( fd, "  start_day_of_week    = %s\n", 
             convArrayToRangeString( mptr->startDaysOfWeekList, 7, 
                                     dayNamesList ) );
    fprintf( fd, "  equilibration_time   = %d\n", mptr->equilibrationTime );
    fprintf( fd, "  aux_sample_direction =" );
    if ( mptr->auxSampleDirection == UP ) {
      fprintf( fd, " up\n" );
    }else if ( mptr->auxSampleDirection == DOWN ) {
      fprintf( fd, " down\n" );
    }else if ( mptr->auxSampleDirection == BOTH ) {
      fprintf( fd, " both\n" );
    }
    fprintf( fd, "  cycles               = %0.1f\n", mptr->cycles );
    fprintf( fd, "  depths               =");
    for ( i = 0; i < mptr->numDepths; i++ ) {
      fprintf( fd, " %d", mptr->depths[i] );
    }
    fprintf( fd, "\n\n");

    mptr = mptr->nextMission;
  }
  fflush( fd );
}
  

//
// NAME
//   parseConfigFile - Open and parse the orcad configuration file.
//
// SYNOPSIS
//   #include "general.h"
//   #include "parser.h"
//
//   int parseConfigFile( char *fileName );
//
// DESCRIPTION
//   Opens and reads the orcad configuration file.  
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int parseConfigFile ( char *fileName ) 
{
  FILE *fpIn;
  int i;
  char buffer[FILEBUFFLEN];
  char *name;
  char *value;
  char *cptr;
  unsigned char in_serial = 0;
  unsigned char in_mission = 0;
  struct mission *lastMission = NULL;
  struct sPort *lastPort = NULL;
  char * token;
  int val;
  int linesRead = 0;
  char * tokenLoc;
  char * valueLoc;

  // Open up the config file
  if ( ( fpIn = fopen(fileName, "r") ) == NULL ) {
    LOGPRINT( LVL_CRIT, "parseConfigFile(): Error - cannot open file "
                        "%s to read.", fileName);
    return( FAILURE );
  }
  
  // Main loop
  while ( 1 ) {
  
    i = getNextConfigTokens( fpIn, buffer, &name, &value, &linesRead );
    if ( i < 0 ) 
    {
      LOGPRINT( LVL_CRIT, "parseConfigFile(): Error on line %d", linesRead );
      return( FAILURE );
    }else if ( i == 0 ) 
    {
      // Done reading the file leave successfully
      break;
    }

    // Parse lines with different token counts
    if ( i == 1 ) {
      if ( strcmp( name, "SERIALIO" ) == 0 ) {
        // Starting a serial defintion
        in_serial = 1;
        in_mission = 0;
        if ( lastPort == NULL ) {
          lastPort = opts.serialPorts = 
                    ( struct sPort * )malloc( sizeof( struct sPort ) );
        } else {
          lastPort->nextPort = 
                    ( struct sPort * )malloc( sizeof( struct sPort ) );
          lastPort = lastPort->nextPort;
          lastPort->nextPort = NULL;
        }
        lastPort->fileDescriptor = -1;
        lastPort->serialID = NULL;
        lastPort->vendorID = 0;
        lastPort->productID = 0;
        lastPort->ftdiContext = NULL;
      }else {
        // Starting a mission definition
        in_mission = 1;
        in_serial = 0;
        if ( lastMission == NULL ) {
          lastMission = opts.missions = 
                    ( struct mission * )malloc( sizeof( struct mission ) );
        } else {
          lastMission->nextMission = 
                    ( struct mission * )malloc( sizeof( struct mission ) );
          lastMission = lastMission->nextMission;
          lastMission->nextMission = NULL;
        }
        // Set defaults
        lastMission->auxSampleDirection = BOTH;
        if ( ( cptr = index(name, '[') ) != NULL ) 
          name = ++cptr;
        if ( ( cptr = rindex(name, ']') ) != NULL ) 
          *cptr = '\0';
        lastMission->name = malloc( (strlen( name )+1) * sizeof( char ) );
        strcpy( lastMission->name, name );
        memset(lastMission->startMinsList, 0, 60);
        memset(lastMission->startHoursList, 0, 24);
        memset(lastMission->startDaysOfMonthList, 0, 32);
        memset(lastMission->startMonthsList, 0, 12);
        memset(lastMission->startDaysOfWeekList, 0, 7);
      }
    }else {
      // Two or more tokens must be a name value pair
      if ( in_serial == 1 ){
        // Serial specific parameters
        if ( strcmp( name, "description" ) == 0 ) {
          lastPort->description = 
                 malloc( (strlen( value )+1) * sizeof( char ) );
          strcpy( lastPort->description, value );
        }else if ( strcmp( name, "type" ) == 0 ) {
          if ( strcmp( value, "SEABIRD_CTD_19" ) == 0 ) {
            lastPort->deviceType = SEABIRD_CTD_19;
          }else if ( strcmp( value, "SEABIRD_CTD_19_PLUS" ) == 0 ) {
            lastPort->deviceType = SEABIRD_CTD_19_PLUS;
          }else if ( strcmp( value, "AGO_METER_WHEEL_COUNTER" ) == 0 ) {
            lastPort->deviceType = AGO_METER_WHEEL_COUNTER;
          }else if ( strcmp( value, "DAVIS_WEATHER_STATION" ) == 0 ) {
            lastPort->deviceType = DAVIS_WEATHER_STATION;
          }else if ( strcmp( value, "ENVIROTECH_ECOLAB" ) == 0 ) {
            lastPort->deviceType = ENVIROTECH_ECOLAB;
          }else if ( strcmp( value, "SATLANTIC_ISIS" ) == 0 ) {
            lastPort->deviceType = SATLANTIC_ISIS;
          }else if ( strcmp( value, "SATLANTIC_ISIS_X" ) == 0 ) {
            lastPort->deviceType = SATLANTIC_ISIS_X;
          }else if ( strcmp( value, "CELL_MODEM" ) == 0 ) {
            lastPort->deviceType = CELL_MODEM;
          }else if ( strcmp( value, "AQUADOPP" ) == 0 ) {
            lastPort->deviceType = AQUADOPP;
          }else if ( strcmp( value, "ARDUINO_METER_WHEEL_COUNTER" ) == 0 ) {
            lastPort->deviceType = ARDUINO_METER_WHEEL_COUNTER;
          }else if ( strcmp( value, "AIRMAR_PB100" ) == 0 ) {
            lastPort->deviceType = AIRMAR_PB100;
          }else if ( strcmp( value, "AIRMAR_PB200" ) == 0 ) {
            lastPort->deviceType = AIRMAR_PB200;
           }else if ( strcmp( value, "GILLMETPAK" ) == 0 ) {
            lastPort->deviceType = GILLMETPAK;
          }else if ( strcmp( value, "RMYOUNGWIND" ) == 0 ) {             
            lastPort->deviceType = RMYOUNGWIND;
          }else if ( strcmp( value, "S9VAISALA" ) == 0 ) {
            lastPort->deviceType = S9VAISALA;
          }else if ( strcmp( value, "SEAFETPH" ) == 0 ) {
            lastPort->deviceType = SEAFETPH;

          }else {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Unknown device type "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "tty" ) == 0 ) {
          if ( strcmp( value, "/dev/ttyS0" ) == 0 ||
               strcmp( value, "/dev/ttyS2" ) == 0 ||
               strcmp( value, "/dev/ttyUSB0" ) == 0  ||
               strcmp( value, "/dev/ttyUSB1" ) == 0  ||
               strcmp( value, "/dev/ttyUSB2" ) == 0  ||
	       strcmp( value, "/dev/ttyUSB3" ) == 0  ||
	       strcmp( value, "/dev/ttyUSB4" ) == 0  ||
	       strcmp( value, "/dev/ttyUSB5" ) == 0  ||
	       strcmp( value, "/dev/ttyUSB6" ) == 0  ||
               strcmp( value, "/dev/ttyUSB7" ) == 0   ) {
            lastPort->tty = malloc( (strlen( value )+1) * sizeof( char ) );
            strcpy( lastPort->tty, value );
          }else {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Unknown tty value "
                      "%s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "baud" ) == 0 ) {
          if ( sscanf(value, "%d", &(lastPort->baud) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading baud value "
                      "%s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "stop_bits" ) == 0 ) {
          if ( sscanf(value, "%d", &(lastPort->stopBits) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading stop_bits "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "data_bits" ) == 0 ) {
          if ( sscanf(value, "%d", &(lastPort->dataBits) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading data_bits "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "flow_control" ) == 0 ) {
          if ( strcmp( value, "rtscts" ) == 0 ) {
            lastPort->flow = FC_RTSCTS;
          }else if ( strcmp( value, "xonxoff" ) == 0 ) {
            lastPort->flow = FC_XONXOFF;
          }else if ( strcmp( value, "none" ) == 0 ) {
            lastPort->flow = FC_NONE;
          }else {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Unknown flow_control "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "parity" ) == 0 ) {
          if ( strcmp( value, "even" ) == 0 ) {
            lastPort->parity = P_EVEN;
          }else if ( strcmp( value, "odd" ) == 0 ) {
            lastPort->parity = P_ODD;
          }else if ( strcmp( value, "none" ) == 0 ) {
            lastPort->parity = P_NONE;
          }else {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Unknown parity "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "vendor_id" ) == 0 ) {
          if ( sscanf(value, "%x", &(lastPort->vendorID) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading vendor_id "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "product_id" ) == 0 ) {
          if ( sscanf(value, "%x", &(lastPort->productID) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading product_id "
                      "value %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "serial_id" ) == 0 ) {
          lastPort->serialID = 
                 malloc( (strlen( value )+1) * sizeof( char ) );
          strcpy( lastPort->serialID, value );
        }else {
          in_serial = 0;
        }
      }else if ( in_mission == 1 ) {
        // Mission specific parameters
        if ( strcmp( name, "schedule" ) == 0 ) {

          // parse date ranges
          if ( ( valueLoc = parseCronField( lastMission->startMinsList, 
                                            60, 0, NULL, value) ) == NULL )
            return( FAILURE );
          if ( ( valueLoc = parseCronField( lastMission->startHoursList, 
                                            24, 0, NULL, valueLoc) ) == NULL )
            return( FAILURE );
          if ( ( valueLoc = parseCronField( lastMission->startDaysOfMonthList,
                                            32, 0, NULL, valueLoc) ) == NULL )
            return( FAILURE );
          if ( ( valueLoc = parseCronField( lastMission->startMonthsList, 
                                12, -1, monthNamesList, valueLoc) ) == NULL )
            return( FAILURE );
          if ( ( valueLoc = parseCronField( lastMission->startDaysOfWeekList, 
                                 7, 0, dayNamesList, valueLoc) ) == NULL )
            return( FAILURE );
          
        }else if ( strcmp( name, "cycles" ) == 0 ) {
          if ( sscanf(value, "%f", &(lastMission->cycles) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "cycles value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "equilibration_time" ) == 0 ) {
          if ( sscanf(value, "%d", &(lastMission->equilibrationTime) ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "equilibration_time value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "aux_sample_direction" ) == 0 ) {
          if ( strcmp( value, "both" ) == 0 ) { 
            lastMission->auxSampleDirection = BOTH;
          }else if ( strcmp( value, "up" ) == 0 ) {
            lastMission->auxSampleDirection = UP;
          }else if ( strcmp( value, "down" ) == 0 ) {
            lastMission->auxSampleDirection = DOWN;
          }else {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "aux_sample_direction value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "depths" ) == 0 ) {
          token = value;
          while ( *token != '\0' ) {
            // Eat whitespace
            while ( *token == ' ' || *token == '\t' ) 
              token++;
            if ( *token != '\0' )
            {
              // Found a token
              lastMission->numDepths++;
              // Eat non whitespace
	      while ( *token != ' ' && *token != '\t' && *token != '\0' ) 
                token++; 
            }
          }
          lastMission->depths = 
              malloc( lastMission->numDepths * sizeof( int ) );
          tokenLoc = value;
          for ( i = 0; i < lastMission->numDepths; i++ ) {
            if ( ( token = strsep( &tokenLoc, " \t\n\r" ) ) != NULL ) {
              if ( sscanf(token, "%d", &val ) < 1 ) {
                LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                          "depth value: %s", value );
                return( FAILURE );
              }
              lastMission->depths[i] = val;
            }
          }
        }else {
          in_mission = 0;
        }
      }
      if ( in_mission != 1 && in_serial != 1 ) {
        // Program general parameters
        if ( strcmp( name, "min_depth" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.minDepth ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "min_depth value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "max_depth" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.maxDepth ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "max_depth value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "parking_depth" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.parkingDepth ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "parking_depth value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "data_file_prefix" ) == 0 ) {
            opts.dataFilePrefix = 
                malloc( (strlen( value )+1) * sizeof( char ) );
            strcpy( opts.dataFilePrefix, value );
        }else if ( strcmp( name, "weather_data_prefix" ) == 0 ) {
            opts.weatherDataPrefix = 
                malloc( (strlen( value )+1) * sizeof( char ) );
            strcpy( opts.weatherDataPrefix, value );
        }else if ( strcmp( name, "weather_sample_period" ) == 0 ) {
          // Deprecated
          //if ( sscanf(value, "%d", &opts.weatherSamplePeriod ) < 1 ) {
          //  LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
          //            "weather_sample_period value: %s", value );
          //  return( FAILURE );
          //}
        }else if ( strcmp( name, "weather_archive_download_period" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.weatherArchiveDownloadPeriod ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "weather_archive_download_period value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "weather_status_filename" ) == 0 ) {
            opts.weatherStatusFilename = 
                malloc( (strlen( value )+1) * sizeof( char ) );
            strcpy( opts.weatherStatusFilename, value );
        }else if ( strcmp( name, "weather_status_update_period" ) == 0 ) {
          // Deprecated
          //if ( sscanf(value, "%d", &opts.weatherStatusUpdatePeriod ) < 1 ) {
          //  LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
          //            "weather_status_update_period value: %s", value );
          //  return( FAILURE );
          //}
        }else if ( strcmp( name, "auxiliary_data_prefix" ) == 0 ) {
            opts.auxiliaryDataPrefix =
                malloc( (strlen( value )+1) * sizeof( char ) );
            strcpy( opts.auxiliaryDataPrefix, value );
        }else if ( strcmp( name, "auxiliary_sample_period" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.auxiliarySamplePeriod ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "auxiliary_sample_period value: %s", value );
            return( FAILURE );         
          }                  
        }else if ( strcmp( name, "auxiliary_archive_download_period" ) == 0 ) {
          if ( sscanf(value, "%d", &opts.auxiliaryArchiveDownloadPeriod ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "auxiliary_archive_download_period value: %s", value );
            return( FAILURE );
          }
       }else if ( strcmp( name, "compass_declination" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.compassDeclination ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "compass_decliation value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "ctd_cal_PA1" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.ctdCalPA1 ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "ctd_cal_PA1 value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "ctd_cal_PA2" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.ctdCalPA2 ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "ctd_cal_PA2 value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "ctd_cal_PA3" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.ctdCalPA3 ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "ctd_cal_PA3 value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "solar_calibration_constant" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.solarCalibrationConstant ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "solar_calibration_constant value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "solar_millivolt_resistance" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.solarMillivoltResistance ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "solar_millivolt_resistance value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "solar_ad_multiplier" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.solarADMultiplier ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "solar_ad_multiplier value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "meterwheel_cfactor" ) == 0 ) {
          if ( sscanf(value, "%lf", &opts.meterwheelCFactor ) < 1 ) {
            LOGPRINT( LVL_CRIT, "parseConfigFile(): Error reading "
                      "meterwheel_cfactor value: %s", value );
            return( FAILURE );
          }
        }else if ( strcmp( name, "data_storage_dir" ) == 0 ) {
            // TODO: Make sure it exists first!
            strncpy( opts.dataDirName, value, FILEPATHMAX );
        }else{
          LOGPRINT( LVL_CRIT, "parseConfigFile(): Unknown "
                    "option %s = %s", name, value );
          return( FAILURE );
        }
      }
    }

  }
  fclose( fpIn );

  return( SUCCESS );

}


// 
// NAME
//   getNextConfigTokens - Read file until one or more tokens are found.
//
// SYNOPSIS
//   #include "general.h"
//   #include "parser.h"
//
//   int getNextConfigTokens( FILE *fIN, char *buffer, char **name,
//                            char **value, int *linesRead );
//
// DESCRIPTION
//   Read from an open file pointed to by fIN until a line
//   is reached containing one or more tokens.  The function
//   should be passed a buffer of FILEBUFFLEN ( see general.h ) 
//   which is used to store the last line read.  The function
//   also increments a file line counter passed in as "linesRead".
//
// RETURNS
//   Upon sucess the function will return the number of tokens
//   read ( currently allows only 1 or 2 per line ) or 0 if the
//   end of file is reached.  The tokens are returned via the
//   "name" and "value" character buffers passed to the function.
//   "name" is used if only one token is on the line.
//
//   In the even of an error the function returns -1.
//
int getNextConfigTokens ( FILE *fIN, char *buffer, char **name, 
                         char **value, int *linesRead ) {
  int ret;
  char bufferDup[FILEBUFFLEN];


  while ( fgets(buffer, FILEBUFFLEN, fIN) ) {
    (*linesRead)++;
    
    strncpy( bufferDup, buffer, FILEBUFFLEN );
    bufferDup[FILEBUFFLEN-1] = '\0';

    if ( ( ret = tokenizeConfigLine( buffer, name, value ) ) < 0 ) {
      LOGPRINT( LVL_CRIT, "getNextConfigTokens(): Syntax error "
                "at %s", bufferDup );
      return ( ret );
    }else if ( ret > 0 ) {
      return( ret );
    }

  }
  return( 0 );

}


//
// NAME
//   advanceToNonWhitespace - Return first non-whitespace character.
//  
// SYNOPSIS
//   #include "parser.h"
//
//   char * advanceToNonWhitespace ( char * cPtr );
//
// DESCRIPTION
//   Return a pointer to the first non-whitespace ( not \t \r \n or ' ' )
//   in the string passed as cPtr ( must be null terminated ).
//
// RETURNS
//   A pointer to a non-whitespace character or to the \0 terminator
//   if non are found.
//
char * advanceToNonWhitespace ( char * cPtr ){
  while ( *cPtr != '\0' &&
          ( *cPtr == '\n' ||
            *cPtr == '\r' ||
            *cPtr == '\t' ||
            *cPtr == ' '  ) ) {
    cPtr++;
  }
  return cPtr;
}


//
// NAME
//   retreatToNonWhitespace - Return last non-whitespace character.
//
// SYNOPSIS
//   #include "parser.h"
//
//   char * retreatToNonWhitespace( char * cPtr, char *bPtr );
//
// DESCRIPTION
//   Return a pointer to the previous non-whitespace character
//   ( not \t \r \n or ' ' ) from cPtr up until bPtr is reached.
//
// RETURNS
//   A pointer to a non-whitespace character or bPtr if
//   non are found.
//
char * retreatToNonWhitespace ( char * cPtr, char *bPtr ){
  if ( bPtr > cPtr ) {
    return cPtr;
  }
  while ( cPtr != bPtr &&
          ( *cPtr == '\n' ||
            *cPtr == '\r' ||
            *cPtr == '\t' ||
            *cPtr == ' '  ) ) {
    cPtr--;
  }
  return cPtr;
}


//
// NAME
//   tokenizeConfigLine - Attempt to break a config line into 1 or 2 tokens.
//
// SYNOPSIS
//   #include "parser.h"
//
//   int tokenizeConfigLine ( char *buffer, char **name, char **value );
//   
// DESCRIPTION
//   Read character buffer and attempt to tokenize the line into
//   either a name token or a name and value tokens.  A config
//   file line may be:
//
//                 : Blank...no tokens generated
//     # Blah Blah : A comment line
//     [foo]       : A name token of "foo" only
//     foo = bar   : A name/value token "foo"/"bar"
//
// RETURNS
//   The number of tokens generated or -1 if an error 
//   occured.
//
int tokenizeConfigLine ( char *buffer, char **name, char **value ) {
  char *tokenEnd;
  char *tokenLoc;

  // The tokenizing routine "strsep" used below will alter
  // the original string passed to it.  If you need to
  // preserve the original string then you should make a 
  // copy first.  In this particular case the original
  // string should be altered.
  tokenLoc = advanceToNonWhitespace(buffer);
  
  // Tokenize
  *name = NULL;
  *value = NULL;

  if ( *tokenLoc == '#' || *tokenLoc == '\0' ) {
    // Just a comment line or an empty line...ignore
    return( 0 ); 
  }

  if ( *tokenLoc == '[' ) {
    tokenLoc++;
    tokenLoc = advanceToNonWhitespace(tokenLoc);
    if ( tokenLoc == '\0' ) {
      // Bad entry!!!
      return( -1 );
    }
    *name = tokenLoc;
    tokenEnd = index( tokenLoc, ']' );
    if ( tokenEnd == NULL ) {
      // Bad entry!!!
      return( -1 );
    }
    tokenEnd--;
    tokenEnd = retreatToNonWhitespace( tokenEnd, tokenLoc );
    if ( tokenLoc == tokenEnd ) {
      // Bad entry!!!
      return( -1 );
    }
    tokenEnd++;
    *tokenEnd = '\0';
    return( 1 );
  }

  if ( ( tokenEnd = index( tokenLoc, '=' )) != NULL ) {
    *name = tokenLoc;
    tokenLoc = tokenEnd + 1;
    tokenEnd = retreatToNonWhitespace( tokenEnd-1, *name );
    if ( *name == tokenEnd ) {
      // Bad entry!!!
      return( -1 );
    }
    tokenEnd++;
    *tokenEnd = '\0';
    
    tokenLoc = advanceToNonWhitespace(tokenLoc);
    if ( tokenLoc == '\0' ) {
      // Bad entry!!!
      return( -1 );
    }
    *value = tokenLoc; 
    tokenEnd = tokenLoc + strlen( tokenLoc ) - 1;
    tokenEnd = retreatToNonWhitespace( tokenEnd, *name );
    tokenEnd++;
    *tokenEnd = '\0';
    return( 2 ); 
  }

  return -1;
}


//
// NAME
//   convArrayToRangeString - Convert a range array into a string.
//
// SYNOPSIS
//   #include "parser.h"
//
//   char * convArrayToRangeString ( char *ary, int arraySize,
//                                   const char *const *names );
//
// DESCRIPTION
//   A range array is a character array where a value of 1
//   indicates that the position is set.  This function reads
//   an array of arraySize and turns this encoding into a
//   human readable range string.
//
//    ie. given:
//
//      ary = 0, 1, 1, 1, 0, 1, 0
//
//    This function returns:
//
//      "1-3,5"
//
// RETURNS
//   A range string or null.
//
char * convArrayToRangeString ( char *ary, int arraySize, 
                                const char *const *names ) 
{
  char *fieldStr = NULL;
  char comma[2];
  int inRange = 0;
  int numChars = 0;
  int i;
  int fieldLen;
  
  // Initialize memory
  *comma = '\0';
  fieldStr = (char * )malloc( sizeof(char) );
  *fieldStr = '\0';

  // Loop through array and find distinct values/ranges
  for ( i = 0; i < arraySize; i++ )
  {
    if ( !inRange && ary[ i ] )
    {
      inRange = i+1;
    }else if ( inRange && !ary[ i ] ) 
    {
      if ( inRange == i )
      {
        // Single value
        if ( names )
        {
          numChars = snprintf( NULL, 0, "%s%s", comma, names[i - 1] );
        }else {
          numChars = snprintf( NULL, 0, "%s%d", comma, i - 1 );
        }
        fieldLen = strlen( fieldStr );
        fieldStr = (char *)realloc( fieldStr, sizeof( char ) * 
                                    ( fieldLen + numChars + 1) );
        if ( names  ) 
        { 
          numChars = snprintf( fieldStr + fieldLen, 
                               numChars + 1, "%s%s", comma, names[i - 1] );
        }else 
        {
          numChars = snprintf( fieldStr + fieldLen, 
                               numChars + 1, "%s%d", comma, i - 1 );
        }
        comma[0] = ','; comma[1] = '\0';
      }else 
      {
        // Range
        if ( names )
        {
          numChars = snprintf( NULL, 0, "%s%s-%s", comma, 
                               names[inRange - 1], names[i - 1] );
        }else {
          numChars = snprintf( NULL, 0, "%s%d-%d", comma,
                               inRange - 1, i - 1 );
        }
        fieldLen = strlen( fieldStr );
        fieldStr = (char *)realloc( fieldStr, sizeof( char ) * 
                                    ( fieldLen + numChars + 1) );
        if ( names  ) 
        { 
          numChars = snprintf( fieldStr + fieldLen, 
                               numChars + 1, "%s%s-%s", comma, 
                               names[inRange - 1], names[i - 1] );
        }else 
        {
          numChars = snprintf( fieldStr + fieldLen, 
                               numChars + 1, "%s%d-%d", comma, 
                               inRange - 1, i - 1 );
        }
        comma[0] = ','; comma[1] = '\0';
      } 
      inRange = 0;
    } 
  }

  if ( inRange == 1 && !strlen( fieldStr ) )
  {
    // * case
    fieldStr = (char *)realloc( fieldStr, sizeof( char ) * 2 );
    fieldStr[0] = '*';
    fieldStr[1] = '\0';
  }else if ( inRange == arraySize )
  {
    // tail value 
    if ( names )
    {
      numChars = snprintf( NULL, 0, "%s%s", comma, names[arraySize - 1] );
    }else {
      numChars = snprintf( NULL, 0, "%s%d", comma, arraySize - 1 );
    }
    fieldLen = strlen( fieldStr );
    fieldStr = (char *)realloc( fieldStr, sizeof( char ) * 
                                ( fieldLen + numChars + 1) );
    if ( names  ) 
    { 
      numChars = snprintf( fieldStr + fieldLen, 
                           numChars + 1, "%s%s", comma, 
                           names[arraySize - 1] );
    }else 
    {
      numChars = snprintf( fieldStr + fieldLen, 
                           numChars + 1, "%s%d", comma, arraySize - 1 );
    }
  }else if ( inRange )
  {
    // tail range
    if ( names )
    {
      numChars = snprintf( NULL, 0, "%s%s-%s", comma, 
                           names[inRange - 1], names[arraySize - 1] );
    }else {
      numChars = snprintf( NULL, 0, "%s%d-%d", comma,
                           inRange - 1, arraySize - 1 );
    }
    fieldLen = strlen( fieldStr );
    fieldStr = (char *)realloc( fieldStr, sizeof( char ) * 
                                ( fieldLen + numChars + 1) );
    if ( names  ) 
    { 
      numChars = snprintf( fieldStr + fieldLen, 
                           numChars + 1, "%s%s-%s", comma, 
                           names[inRange - 1], names[arraySize - 1] );
    }else 
    {
      numChars = snprintf( fieldStr + fieldLen, 
                           numChars + 1, "%s%d-%d", comma, 
                           inRange - 1, arraySize - 1 );
    }
  }

  return( fieldStr );
}


//
// NAME
//   parseCronField - Parse a schedule line and convert it to an array.
//
// SYNOPSIS
//   #include "parser.h"
//
//   char * parseCronField( char *ary, int modvalue, int off,
//                          const char *const *names, char *ptr );
//
// DESCRIPTION
//   This routine is culled from the BusyBox version of
//   chrond. Hats off to them.  Basically this routine
//   parses a standard chron schedule fields and converts
//   them to byte mask arrays for each field.
//
// RETURNS
//   The remains of the schedule line or NULL.
//
char * parseCronField ( char *ary, int modvalue, int off,
                        const char *const *names, char *ptr )
{
  char *base = ptr;
  int n1 = -1;
  int n2 = -1;

  if (base == NULL) {
    return (NULL);
  }

  while (*ptr != ' ' && *ptr != '\t' && *ptr != '\n') {
    int skip = 0;

    /* Handle numeric digit or symbol or '*' */

    if (*ptr == '*') {
      n1 = 0;    /* everything will be filled */
      n2 = modvalue - 1;
      skip = 1;
      ++ptr;
    } else if (*ptr >= '0' && *ptr <= '9') {
      if (n1 < 0) {
        n1 = strtol(ptr, &ptr, 10) + off;
      } else {
        n2 = strtol(ptr, &ptr, 10) + off;
      }
      skip = 1;
    } else if (names) {
      int i;

      for (i = 0; names[i]; ++i) {
        if (strncasecmp(ptr, names[i], strlen(names[i])) == 0) {
          break;
        }
      }
      if (names[i]) {
        ptr += strlen(names[i]);
        if (n1 < 0) {
          n1 = i;
        } else {
          n2 = i;
        }
        skip = 1;
      }
    }

    /* handle optional range '-' */

    if (skip == 0) {
      LOGPRINT( LVL_WARN, "parseCronField(): Failed parsing config " 
                "line/field: %s", base );
      return ( NULL );
    }
    if (*ptr == '-' && n2 < 0) {
      ++ptr;
      continue;
    }

    /*
     * collapse single-value ranges, handle skipmark, and fill
     * in the character array appropriately.
     */

    if (n2 < 0) {
      n2 = n1;
    }
    if (*ptr == '/') {
      skip = strtol(ptr + 1, &ptr, 10);
    }
    /*
     * fill array, using a failsafe is the easiest way to prevent
     * an endless loop
     */

    {
      int s0 = 1;
      int failsafe = 1024;

      --n1;
      do {
        n1 = (n1 + 1) % modvalue;

        if (--s0 == 0) {
          ary[n1 % modvalue] = 1;
          s0 = skip;
        }
      }
      while (n1 != n2 && --failsafe);

      if (failsafe == 0) {
        LOGPRINT( LVL_WARN, "parseCronField(): Failsafe reached!  "
                  "Failed parsing config line/field: %s", base );
        return ( NULL );
      }
    }
    if (*ptr != ',') {
      break;
    }
    ++ptr;
    n1 = -1;
    n2 = -1;
  }

  if ( *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\0' ) {
    LOGPRINT( LVL_WARN, "parseCronField(): Could not find field boundry!  "
              "Failed parsing config line/field: %s", base );
    return ( NULL );
  }

  while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' ) {
    ++ptr;
  }

  return (ptr);

}


//
// NAME
//   fixDayDow - Reconcile differences in Day and Day of Week conflicts.
//
// SYNOPSIS
//   #include "parser.h"
//
//   fixDayDow(struct mission * misn);
//
// DESCRIPTION
//   Culled from the BusyBox version of crond.  Hats off to them.
//   Reconciles the difference between two conflicting Day and
//   Day of Week specifications in a cron schedule array.
//
// RETURNS
//   Nothing
//
void fixDayDow (struct mission * misn)
{
  short i;
  short weekUsed = 0;
  short daysUsed = 0;

  for (i = 0; i < 7; ++i) {
    if (misn->startDaysOfWeekList[i] == 0) {
      weekUsed = 1;
      break;
    }
  }
  for (i = 0; i < 32; ++i) {
    if (misn->startDaysOfMonthList[i] == 0) {
      daysUsed = 1;
      break;
    }
  }
  if (weekUsed && !daysUsed) {
    memset(misn->startDaysOfMonthList, 0, 32);
  }
  if (daysUsed && !weekUsed) {
    memset(misn->startDaysOfWeekList, 0, 7);
  }
}
