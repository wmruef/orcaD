/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * iotest.c : A test program for the bitsyX IO
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
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <hardio.h>


// logPrint is called by routines in hardio.h to 
// record the status of the io routines.  Instead of 
// opening up a real log file for this program we
// are simply overriding log.c with a generic print
// routine.
void logPrint ( int level, char *message, ... )
{
  va_list ap;
  va_start( ap, message );
  if ( level < 5 )
    vprintf( message, ap );
  va_end( ap );
}

//
//  At some point in the future I will make a very lowlevel version
//  of iotest which will allow you to do whatever you want with the
//  bits.  For now it would only be a dangerous option for the masses
//  and should be left out.
//
//"     or\n"
//"       iotest PORTA|PORTB|PORTC  OR|AND  10101010\n\n",
// "     iotest PORTC OR 00000001     Turns on power to the winch\n",
// "                                  while leaving PC1-PC7\n",
// "                                  in their original state.\n\n",
// "     iotest PORTC AND 01111110    Turns off the winch while\n",
// "                                  leaving PC1-PC7 in their original\n",
// "                                  state.  NOTE: While in this example PC7\n",
// "                                  and'd with 0 this will not change the\n",
// "                                  state of this *unavailble* port\n\n",
  
static char *options[] = {
 "\n\nusage: iotest [help] [ WINCHPWR|WINCHDIR|HYDROWIRE|WEATHER|\n",
 "                       METER|WIFI|ODROID|ZOOCAM  ON|1|OFF|0 ]\n",
 "              [ INTVOLT|EXTVOLT|SOLARVOLT|STATUS ]\n\n",
 "  This program allows you to manipulate the current output state\n",
 "  of the Raspberry PI/bitsyXb/bitsyX IO ports.\n\n"
 "  Examples:\n",
 "     iotest WINCHPWR 1            Turns on the winch by setting\n",
 "                                  setting the IO pin high.\n",
 "     iotest WIFI OFF              Turn off the comm port by setting\n",
 "                                  the IO pin low.\n\n",
 "     iotest INTVOLT               Read and display the scaled A/D value\n",
 "                                  corresponding to the internal\n",
 "                                  voltage.\n\n",
 "     iotest STATUS                Show the status of all the defined\n",
 "                                  IO ports and A/D ports\n",
 " To see more documentation one page at a time type:\n\n",
 "     iotest help | more\n\n",
 " Robert Hubley <rhubley@gmail.com>, 2005-2014\n\n",
 0,
};
 

static char *details[] = {
 "\n\n Raspberry PI with PI Filling IO Board\n\n",
 "",
 "\n\n BitsyXb/BitsyX SMARTIO System:\n\n",
 "    Several assumptions are made here:\n\n",
 "       - The smartIO system has not been pre-initialized by any\n",
 "         other software following the last boot.  This ensures\n",
 "         that this program has the ability to initialize the\n",
 "         SmartIO system; something which can only be done once\n",
 "         following a powerup.\n\n",
 "       - The initilialization of the SmartIO system to mode 2\n",
 "         doesn't free up all IO lines controlled by ports A-D\n",
 "         so attempting to set the disallowed lines will be\n",
 "         silently ignored.\n\n",
 "       - All lines are set to output only.\n\n",
 "    SmartIO Port Setup:\n",
 "              Pin Signal Alias  Device\n",
 "              --- ------ -----  ------\n",
 "     Port A:  31  PA0    Col 0  External Voltage A/D\n",
 "              33  PA1    Col 1  Internal Voltage A/D\n", 
 "              35  PA2    Col 2  WIFI Amplifier Power I/O\n", 
 "              37  PA3    Col 3  Solar Radiation A/D\n", 
 "                  PA4-PA7       Unused\n", 
 "     Port B:      PB0           Unused\n",
 "                  PB1           Unused\n", 
 "                  PB2           Unused\n", 
 "                  PB3           Unused\n", 
 "                  PB4-PB7       **Unavailable**\n", 
 "     Port C:  30  PC0    Row 0  Winch Power ( high = on )\n",
 "              32  PC1    Row 1  Winch Direction ( high = up )\n",
 "              34  PC2    Row 2  HydroWire Power ( high = on )\n",
 "              36  PC3    Row 3  Weather Station Power ( high = on )\n",
 "              38  PC4    Row 4  Meter Wheel Power ( high = on )\n",
 "                  PC5    Row 5  **Possibly Unavailable**\n",
 "                  PC6    Row 6  **Unavailable**\n",
 "                  PC7    Row 7  **Unavailable**\n",
 "     Port D:      PD0           **Possibly Unavailable**\n",
 "                  PD1           **Possibly Unavailable**\n",
 "                  PD2-7         **Unavaliable**\n\n",
 0,
};

void usage ( char *message )
{
  int i;
  char *s;
  for( i = 0; (s = options[i]); ++i ){
      printf( s );
  }
  if ( message != NULL ) {
    printf ("%s\n\n", message );
  }
  exit(1);
}

void help ()
{
  int i;
  char *s;
  for( i = 0; (s = options[i]); ++i ){
      printf( s );
  }
  for( i = 0; (s = details[i]); ++i ){
      printf( s );
  }
  exit(1);
}
 

int main(int argc, char *argv[], char *envp[])
{
  int i;
  char message[256];
  int ret;
  char state = 0;
  char *argPtr = NULL;
  float val;

  if ( initializeIO() < 0 )
  {
    fprintf( stderr, "Error in initializeIO()!\n" );
  }

  if ( argc < 2 || argc > 3  )
    usage(""); 

  if ( argc == 3 ) {
    // Second argument
    argPtr = argv[2];
    if ( strcasecmp( argPtr, "on") == 0 ||
         strcmp( argPtr, "1" ) == 0 ) {
      state = 1;
    }else if ( strcasecmp( argPtr, "off") == 0 ||
               strcmp( argPtr, "0" ) == 0 ) {
      state = 0;
    }else {
      sprintf( message, "===> Incorrect argument %s!", argPtr );
      usage( message );
    }
    // First argument
    argPtr = argv[1];
    //fprintf( stderr, "argv[1] = %s\n", argPtr );
    if ( strcasecmp( argPtr, "winchpwr") == 0 ) {
      ret = WINCH_PWR_STATUS;
      if ( state )  
        WINCH_ON;
      else 
        WINCH_OFF;
      printf("iostat: Winch power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "winchdir") == 0 ) {
      ret = WINCH_DIR_STATUS;
      if ( state )  
        WINCH_UP;
      else 
        WINCH_DOWN;
      printf("iostat: Winch direction changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "hydrowire") == 0 ) {
      ret = HYDRO_STATUS;
      if ( state ) 
        HYDRO_ON;
      else 
        HYDRO_OFF;
      printf("iostat: Hydro power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "weather") == 0 ) {
      ret = WEATHER_STATUS;
      if ( state )
        WEATHER_ON;
      else 
        WEATHER_OFF;
      printf("iostat: Weather station power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "meter") == 0 ) {
      ret = METER_STATUS;
      if ( state )
        METER_ON;
      else
        METER_OFF;
      printf("iostat: Meter wheel power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "wifi") == 0 ) {
      ret = WIFI_STATUS;
      if ( state ) 
        WIFIAMP_ON;
      else 
        WIFIAMP_OFF;
      printf("iostat: WIFI amplifier power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "odroid") == 0 ) {
      ret = ODROID_STATUS;
      if ( state )
        ODROID_ON;
      else
        ODROID_OFF;
      printf("iostat: ODROID power changed from %d to %d.\n", ret, state );
    }else if ( strcasecmp( argPtr, "zoocam") == 0 ) {
      ret = ZOOCAM_STATUS;
      if ( state )
        ZOOCAM_ON;
      else
        ZOOCAM_OFF;
      printf("iostat: ZOOCAM power changed from %d to %d.\n", ret, state );
    }else {
      sprintf( message, "===> Incorrect argument %s!", argPtr );
      usage( message );
    } 
  }else if ( argc == 2 ) {
    // First argument
    argPtr = argv[1];
    if ( strcasecmp( argPtr, "help") == 0  ) {
      help();
    }else if ( strcasecmp( argPtr, "intvolt") == 0 ) {
      val = GET_INTERNAL_BATTERY_VOLTAGE;
      printf("iostat: Internal Voltage = %6.6f volts\n", val );
    }else if ( strcasecmp( argPtr, "extvolt") == 0 ) {
      val = GET_EXTERNAL_BATTERY_VOLTAGE;
      printf("iostat: External Voltage = %6.6f volts\n", val );
    }else if ( strcasecmp( argPtr, "solarvolt") == 0 ) {
      val = GET_SOLAR_RADIATION_VOLTAGE;
      printf("iostat: Solar Radiation Voltage = %6.6f volts\n", val );
    }else if ( strcasecmp( argPtr, "status") == 0 ) {
      ret = WINCH_PWR_STATUS;
      printf("iostat: Winch Power     = %d\n", ret );
      ret = WINCH_DIR_STATUS;
      printf("        Winch Direction = %d\n", ret );
      ret = HYDRO_STATUS;
      printf("        Hydrowire Power = %d\n", ret );
      ret = WEATHER_STATUS;
      printf("        Weather Power   = %d\n", ret );
      ret = METER_STATUS;
      printf("        Meterwheel Power= %d\n", ret );
      ret = WIFI_STATUS;
      printf("        WIFI Power      = %d\n", ret );
      val = GET_INTERNAL_BATTERY_VOLTAGE;
      printf("        Internal Voltage        = %6.6f volts\n", val );
      val = GET_EXTERNAL_BATTERY_VOLTAGE;
      printf("        External Voltage        = %6.6f volts\n", val );
      val = GET_SOLAR_RADIATION_VOLTAGE;
      printf("        Solar Radiation Voltage = %6.6f volts\n\n", val );

      printf(" Raw ADC Voltages: \n");
      printf("        port 0 = %6.6f volts\n", getADVoltage(0) );
      printf("        port 1 = %6.6f volts\n", getADVoltage(1) );
      printf("        port 2 = %6.6f volts\n", getADVoltage(2) );
      printf("        port 3 = %6.6f volts\n", getADVoltage(3) );
      printf("        port 4 = %6.6f volts\n", getADVoltage(4) );
      printf("        port 5 = %6.6f volts\n", getADVoltage(5) );
      printf("        port 6 = %6.6f volts\n", getADVoltage(6) );
      printf("        port 7 = %6.6f volts\n", getADVoltage(7) );

    }else {
      sprintf( message, "===> Incorrect argument %s!", argPtr );
      usage( message );
    }
  }else {
    if ( strcasecmp( argPtr, "porta") == 0 ) {
    }else if ( strcasecmp( argPtr, "portb") == 0 ) {
    }else if ( strcasecmp( argPtr, "portc") == 0 ) {
    }else {
      sprintf( message, "===> Incorrect argument %s!", argPtr );
      usage( message );
    }
    // Second argument
    argPtr = argv[2];
    //fprintf( stderr, "argv[2] = %s\n", argPtr );
    if ( strcasecmp( argPtr, "and") == 0 ) {
    }else if ( strcasecmp( argPtr, "or") == 0 ) {
    }else {
      sprintf( message, "===> Incorrect argument %s!", argPtr );
      usage( message );
    }
  
    // Third argument
    argPtr = argv[3];
    fprintf( stderr, "argv[3] = %s\n", argPtr );
    if ( strlen( argPtr ) != 8 ) {
      sprintf( message, "===> Binary data ( %s ) must be 8 bits long!", argPtr );
      usage( message );
    }
    for ( i = 0; i < strlen( argPtr ); i++ ) {
      if ( argPtr[i] != '0' && argPtr[i] != '1' ) {
        sprintf( message, "===> Binary data ( %s ) must be contain only 1 and 0's", argPtr );
        usage( message );
      } 
    }

  }

  return( 1 );

}



