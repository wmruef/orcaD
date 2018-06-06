/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * pifilling.c : The PI Filling Raspberry Pi I/O routines
 *
 * Created: July 2014
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h> 
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "general.h"
#include "orcad.h"
#include "term.h"
#include "log.h"
#include "hydro.h"
#include "aquadopp.h"
#include "hardio.h"
#include "weather.h"


int initializeIO () {
  int file;

  // Say hi
  LOGPRINT( LVL_VERB, "intializeIO(): Entered" );

  char *filename = "/dev/i2c-1";
  if (( file = open(filename, O_RDWR )) < 0 ) 
  {
    return( FAILURE );
  }

  // Configure DIO Ports
  if ( ioctl(file, I2C_SLAVE, PI_FILLING_MCP23017_ADDR ) < 0 )
  {
    close(file);
    return( FAILURE );
  }

  struct i2c_smbus_ioctl_data args;
  union i2c_smbus_data data;

  // Configure DIO output ports
  args.read_write = I2C_SMBUS_WRITE;
  args.command = MCP23017_IODIRA;
  args.size = I2C_SMBUS_WORD_DATA;
  args.data = &data;
  data.byte = MCP23017_IODIRA_DIRMASK;
  ioctl( file, I2C_SMBUS, &args );
  close(file);
  return( SUCCESS );
}

// 
// NAME
//   getOutputLine - get the state of a SMARTIO output line.
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   int getOutputLine( char port, char line );
//
// DESCRIPTION
//   Query the state ( 0/1 ) of the output line of the
//   DIO chip on the Pi Filling board.
//   
// RETURNS
//   State of the given output line or -1 upon failure.
//
int getOutputLine ( char line ) 
{
  int file;

  //printf("Starting...\n");

  char *filename = "/dev/i2c-1";
  if (( file = open(filename, O_RDWR )) < 0 ) {
    //perror("Failed to open the i2c bus");
    return( FAILURE );
  }
  //printf("  - opened /dev/i2c-1\n");


  // TODO Make a contstant
  int device_addr = 0x24;
  if ( ioctl(file, I2C_SLAVE, device_addr) < 0 )
  {
   //printf("Failed to acquire bus access and/or talk to the slave.\n");
   close(file);
   return( FAILURE );
  }
  //printf( "  - set slave address = %x\n", device_addr);

  struct i2c_smbus_ioctl_data args;
  union i2c_smbus_data data;
  

  // Read current state of port
  args.read_write = I2C_SMBUS_READ;
  args.command = MCP23017_GPIOA;
  args.size = I2C_SMBUS_WORD_DATA;
  args.data = &data;
  data.byte = 0x00;
  ioctl( file, I2C_SMBUS, &args );
  close( file );
  return ( ( ( 0x01 << line ) & data.byte) >> line  );

}




// 
// NAME
//   setOutputLine - Set the state of a SMARTIO output line.
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   int setOutputLine( char line, char state );
//
// DESCRIPTION
//   Change the state ( 0/1 ) of the output line of a 
//   port.  
//
//   NOTE: Function used in critical areas...do not log!
//
// RETURNS
//   -1 Upon failure
//    1 Upon success
//
int setOutputLine ( char line, char state ) 
{
  char outputPortMask = 0x00;
  int file;
  char outMask, newState;

  //printf("Starting...\n");

  char *filename = "/dev/i2c-1";
  if (( file = open(filename, O_RDWR )) < 0 ) {
    //perror("Failed to open the i2c bus");
    return( FAILURE );
  }
  //printf("  - opened /dev/i2c-1\n");


  // TODO Make a contstant
  int device_addr = 0x24;
  if ( ioctl(file, I2C_SLAVE, device_addr) < 0 )
  {
   //printf("Failed to acquire bus access and/or talk to the slave.\n");
   close( file );
   return( FAILURE );
  }
  //printf( "  - set slave address = %x\n", device_addr);


  struct i2c_smbus_ioctl_data args;
  union i2c_smbus_data data;

  args.size = I2C_SMBUS_WORD_DATA;
  args.data = &data;

  // read port directions
  args.read_write = I2C_SMBUS_READ;
  args.command = MCP23017_IODIRA;
  data.byte = 0x00;
  ioctl( file, I2C_SMBUS, &args );
  outputPortMask = data.byte;


  // Need to read first so we don't change the state of any other line
  // Read current state of port
  args.read_write = I2C_SMBUS_READ;
  args.command = MCP23017_GPIOA;
  data.byte = 0x00;
  ioctl( file, I2C_SMBUS, &args );
  //printf("Current state of all output lines is %x\n", data.byte );


  outMask = ( 0x01 << line) & ~outputPortMask;  
  if ( state ) {
    newState = data.byte | outMask;
  }else {
    newState = data.byte & ~outMask;
  }
 
  // Write
  args.read_write = I2C_SMBUS_WRITE;
  args.command = MCP23017_GPIOA;
  data.byte = newState;
  ioctl( file, I2C_SMBUS, &args );
  
  close( file );
  return( SUCCESS );
}


//
// NAME
//   emergencyPortClear - clear all port lines to low -- quickly.
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   int emergencyPortClear( char * port );
//
// DESCRIPTION
//   A simple routine to set all port lines to low without
//   too much overhead.  This is used when attempting to 
//   shutdown mechanical hardware ( such as the winch )
//   quickly after something goes wrong.
//
// RETURNS
//   Signal kind ( not quite safe ) 
//   routine to set port lines to low
//
int emergencyPortClear ( char * port )
{
  return( 1 );
}

//
// NAME
//   getADValue - Get the analog signal raw ADC value
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   long getADValue( char line );
//
// DESCRIPTION
//   Open the A/D port on the bitsyx and read the
//   A/D line.  
//   lines 0-3 on chip #1
//   lines 4-7 on chip #2
//
// RETURNS
//   The raw value from the ADC or -1 if something
//   went wrong.
//
long getADValue ( char line ) {
  int file;
  long value;

  // Say hi
  LOGPRINT( LVL_VERB, "getADValue(): Entered" );

  // We only have 8 A/D lines
  if ( line < 0 || line > 7 )
    return( FAILURE );

  char *filename = "/dev/i2c-1";
  if (( file = open(filename, O_RDWR )) < 0 ) {
    //perror("Failed to open the i2c bus");
    return( FAILURE );
  }
 
  int device_addr = PI_FILLING_MCP3424_1_ADDR;

  if ( line > 3 )
  {
    line = line - 4;
    device_addr = PI_FILLING_MCP3424_2_ADDR;
  }
 
  char configVal = 0;
  configVal = ((line & 0x03) << 5);
  configVal = configVal | MCP3424_PGA_X1;
  configVal = configVal | MCP3424_18BIT;
  configVal = configVal | MCP3424_SAMPLE_CONTINUOUS;
  configVal = configVal | MCP3424_START_CONV;

  if ( ioctl(file, I2C_SLAVE, device_addr) < 0 )
  {
   //printf("Failed to acquire bus access and/or talk to the slave.\n");
   close(file);
   return( FAILURE );
  }
  //printf( "  - set slave address = %x and configVal = %x\n", device_addr, configVal);

  char buf[10] = {0};
  buf[0] = configVal;
  if ( write(file,buf,1) != 1 )
  {
    printf("Failed to write to the i2cbus!\n");
  }

  int counter = 5000;
  while ( counter )
  {
    if ( read(file,buf, 4 ) != 4 )
    {
      printf("Failed to read the i2cbus!\n");
    }
 
    if ( (buf[3] & 0x80) == 0 )
    {
//      printf("value = %x\n", buf[0]);
//      printf("value = %x\n", buf[1]);
//      printf("value = %x\n", buf[2]);
//      printf("value = %x\n", buf[3]);
      break;
    }
    counter--;
  }
  // close file
  close(file);

  if ( counter == 0 || ( buf[3] & 0x80) != 0 )
  {
    // Error occured!
    LOGPRINT( LVL_VERB, "getADValue(): Error reading A/D converter");
    return ( 0 );
  }

  // Resolve what this functions return value should be
  value = (( buf[0] & 0b00000001 ) << 16 ) | ( buf[1] << 8 ) | buf[2];
  if ( buf[0] & 0x02 ) 
    value = ~(0x020000 - value);

  LOGPRINT( LVL_VERB, "getADValue(): Returning: %d", value );
  return( value );
}


//
// NAME
//   getADVoltage - Get the analog signal voltage at a A/D port scaled.
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   float getADScaledValue( char line );
//
// DESCRIPTION
//   Open the A/D port on the pi filling and read the
//   A/D line raw value and scale it. 
//   NOTE: Assumes 18bit and standard PGA.
//
// RETURNS
//   The raw voltage on the line or -1 if something
//   went wrong.  
//
float getADVoltage ( char line ) 
{
  long r;
  float ret = 0.0;

  // Say hi only
  LOGPRINT( LVL_VERB, "getADVoltage(): Entered" );

  r = getADValue( line );
  if ( r == -1 )
    return ( -1.0 );

  // The constant used here to convert the A/D raw
  // value to a voltage is based on the MCP3424 chip
  // setup as 18bit, 2x gain.  See 
  // https://github.com/uChip/MCP342X/blob/master/MCP342X.cpp
  // for details.
  // voltage = raw * ( lsb / pga );
  ret = (float)r * ( (float)0.0000078125 / (float)0.5 );

  return ( ret );
}


// DEPRECATED....do not use this function anymore
//
// NAME
//   getADScaledValue - Get the analog signal magnitude at a A/D port scaled.
//
// SYNOPSIS
//   #include "pifilling.h"
//
//   float getADScaledValue( char line );
//
// DESCRIPTION
//   Open the A/D port on the pi filling and read the
//   A/D line raw value and scale it. 
//
// RETURNS
//   The raw voltage on the line scaled or -1 if something
//   went wrong.
//
float getADScaledValue ( char line, float scale ) 
{
  long r;
  float ret = 0.0;

  // Say hi only
  LOGPRINT( LVL_VERB, "getADScaledValue(): Entered" );

  r = getADValue( line );
  if ( r == -1 )
    return ( -1.0 );
  // 2^18 = 262144 levels with a 2.048V max input.
  ret = (float)r * ( 2.048/262144.0 ) / ( 1.0 / 2.048 );
  //printf( "Final %0.8f\n\n", ret );
  return ( ret * scale );
}


