/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * buoy.c : Routines associated with the buoy hardward configuration
 *
 * Created: August 2010
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
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>

#ifndef RASPPI
#include <ftdi.h>
#endif

#include "general.h"
#include "orcad.h"
#include "term.h"
#include "log.h"
#include "hydro.h"
#include "aquadopp.h"
#include "weather.h"
#include "hardio.h"
#include "buoy.h"


// 
// NAME
//   initializeHardware - Initialize the buoy hardware
//
// SYNOPSIS
//   #include "buoy.h"
//
//   int initializeHardware();
//
// DESCRIPTION
//   Setup the BitsyX processor board prior to use with the OrcaD
//   package.  This is called prior to doing any I/O operations.
//   
//   The basic intializations are:
//
//     - Set the SMARTIO option to SMARTIO_INIT_OPTION as defined
//       in buoy.h.  Currently this is set to 2 which indicates
//       tot SMARTIO system that PORT A should be an A/D converter
//       and all other ports as I/O.
//
//     - This resets the winch power to off and direction to up ( a
//       low voltage on the direction port ). 
//
//     - Turns on external hardware if defined in the config file.
//       This includes the hydro wire power, the weather station power,
//       and the meterwheel power.
//
//     - Initialize the communication ports for devices which 
//       are defined in the config file.  This includes the hydro
//       device, the weather station, and the meterwheel.
//
// RETURNS
//   -1 Upon failure
//    1 Upon Success
//
int initializeHardware () {
  int r, wsFD;
  int hydroType;
  int hydroFD;

  // Say hi
  LOGPRINT( LVL_VERB, "intializeHardware(): Entered" );

  //
  // Initialize SMARTIO
  //
  if ( initializeIO() == FAILURE )
  {
    LOGPRINT( LVL_WARN, 
              "initializeHardware(): Error initializing SMARTIO system!" ); 
    return( FAILURE );
  }

  // Clear the winch outputs just to be safe
  //Oops...winch down is logic zero...and keeps the FETs/Solenoids off!
  //WINCH_UP;
  WINCH_DOWN;
  WINCH_OFF;

  // Initialize the term library
  r = term_lib_init();
  if ( r < 0 )
  {
    LOGPRINT( LVL_ALRT, 
              "initializeHardware(): term_init failed: %s", 
              term_strerror(term_errno, errno));
    return( FAILURE );
  }

  //
  // Turn on external hardware if present
  //
  if ( ( hydroType = getHydroWireDeviceType() ) > -1 )
  {
    // WMR 10/16/13: All buoy's have been switched from an 
    //               underwater battery pack to a supercapacitor
    //               bank.  Therefore, there is no need to keep
    //               the power on the cable while we are not
    //               profiling.
    // LOGPRINT( LVL_INFO, "initializeHardware(): Turning on hydrowire power" );
    // HYDRO_ON;
 
    // Open up the serial port
    if( ( hydroFD = getDeviceFileDescriptor( hydroType ) ) < 0 ) 
    {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not open up the "
                "hydro device serial port!" );
      return( FAILURE );
    }
    if ( initHydro( hydroType, hydroFD ) < 0 ) {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not initialize "
                "the hydro device!" );
      return( FAILURE );
    }
  }

  if ( hasSerialDevice( DAVIS_WEATHER_STATION ) > 0 )
  { 
    // Turn on
    LOGPRINT( LVL_INFO, "initializeHardware(): Turning on weather station " 
              "power" );
    WEATHER_ON;

    // Give it some time to initialize
    sleep( 3 );

    // Open up the serial port
    if( ( wsFD = getDeviceFileDescriptor( DAVIS_WEATHER_STATION ) ) < 0 ) 
    {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not open up the "
                "weather station serial port!" );
      return( FAILURE );
    }

    if ( initializeWeatherStation( wsFD ) < 0 ) 
    {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not initialize the "
                "weather station!" );
      return( FAILURE );
    }

  }

  if ( hasSerialDevice( AGO_METER_WHEEL_COUNTER ) > 0
       || 
       hasSerialDevice( ARDUINO_METER_WHEEL_COUNTER ) > 0 ) 
  {
    // Turn it on
    METER_ON;
    // TODO: Record it's base position
  }

  if ( hasSerialDevice( AQUADOPP ) > 0 )
  {
    LOGPRINT( LVL_INFO, "initializeHardware(): Initializing the " 
              "Aquadopp." );

    // Open up the serial port
    if( ( wsFD = getDeviceFileDescriptor( AQUADOPP ) ) < 0 ) 
    {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not open up the "
                "aquadopp serial port!" );
      return( FAILURE );
    }

    // initialize it.
    if ( initAquadopp( wsFD ) < 0 )
    {
      LOGPRINT( LVL_ALRT, "intializeHardware(): Could not initialize "
                "aquadopp hardware!" );
      return( FAILURE );
    }
  }

  // Say goodbye
  LOGPRINT( LVL_VERB, "intializeHardware(): Returning: SUCCESS" );

  return( SUCCESS );
}


//
// NAME
//   hasSerialDevice - Do we have a serial config for a particular device
//
// SYNOPSIS
//   #include "buoy.h"
//
//   int hasSerialDevice( int deviceType );
//
// DESCRIPTION
//   A helper function which queries the configuration ( set by the
//   config file ) for a serial port configuration given it's device
//   type.  
//
// RETURNS
//   -1 If no device exists
//    1 If device exists and is a standard serial device
//    2 If device exists and is a USB serial device
//
int hasSerialDevice ( int deviceType ) 
{
  struct sPort *port;

  // Say hi
  LOGPRINT( LVL_VERB, "hasSerialDevice( %d ): Entered", deviceType );

  port = opts.serialPorts;
  while ( port != NULL ) 
  {
    if ( port->deviceType == deviceType  )
    {
      // TODO: A kludgy test for serial type.  This should be a member
      // of the sPort structure!
      if ( port->vendorID != 0 )
      {
        LOGPRINT( LVL_VERB, 
                 "hasSerialDevice(): Returning true ( 2 )" );
        return( 2 );
      }else 
      {
        LOGPRINT( LVL_VERB, 
                 "hasSerialDevice(): Returning true ( 1 )" );
        return( 1 );
      }
   }
    port = port->nextPort;
  }
  
  LOGPRINT( LVL_VERB, 
            "hasSerialDevice(): Returning false ( -1 )" );
  return ( -1 );
}

//
// NAME
//   getSerialDevice - get sPort structure for a given serial device type
//                     and initialize device if necessary.
//
// SYNOPSIS
//   #include "buoy.h"
//
//   struct sPort * getSerialDevice( int deviceType );
//
// DESCRIPTION
//   Find the sPort structure entry for a given serial device type and
//   check to see if the port has been opened...if not open it.  Returns
//   the sPort entry complete with the opened device. 
//
// RETURNS
//    * to the sPort structure or NULL if it doesn't exist or cannot be
//    opened.
//
struct sPort *getSerialDevice ( int deviceType ) 
{
  struct sPort *port;

  // Say hi
  LOGPRINT( LVL_VERB, "getSerialDevice(): Entered" );

  port = opts.serialPorts;
  while ( port != NULL ) 
  {
    if ( port->deviceType == deviceType  )
    {
      if ( port->vendorID != 0 )
      {
        if ( port->ftdiContext == NULL )
        {
          // We need to open the USB port before returning
          if ( getUSBDeviceContext( deviceType ) == NULL )
          {
            return( NULL );
          }
        }
        // We have a USB port and it's opened
        return( port );
      }else if ( port->tty != NULL )
      {
        if ( port->fileDescriptor == -1 )
        {
          // We need to open the serial port before returning
          if ( getDeviceFileDescriptor( deviceType ) == -1 )
          {
            return( NULL );
          }
        }
        // We have a standard serial port and it's opened
        return( port );
      }else
      {
        LOGPRINT( LVL_WARN, 
              "getSerialDevice(): Could not determine the "
              "the serial device type ( usb/serial )!" );
        return( NULL );
      }
    }
    port = port->nextPort;
  }
  
  LOGPRINT( LVL_VERB, 
            "getSerialDevice(): Returning false ( NULL )" );
  return ( NULL );
}


//
// NAME
//   getHydroWireDeviceType - Get the device type for hydro wire communication
//
// SYNOPSIS
//   #include "buoy.h"
//
//   int getHydroWireDeviceType();
//
// DESCRIPTION
//   Determine which type of hydro device is supposed to be 
//   on the other end of the hydro wire.  This is obtained
//   by looking at the config file.  There is no check to 
//   see if this is the actual device on the other end.
//
// RETURNS
//   The device type on the other end of the hydro wire.
//   Currently one of:
//     SEABIRD_CTD_19
//     SEABIRD_CTD_19_PLUS 
//     ENVIROTECH_ECOLAB
//     SATLANTIC_ISIS
//     SATLANTIC_ISIS_X
//
//   See buoy.h for device types
//   This function will return -1 if it fails.
//
int getHydroWireDeviceType ()
{
  struct sPort *port;

  // Say hi
  LOGPRINT( LVL_VERB, "getHydroWireDeviceType(): Entered" );

  port = opts.serialPorts;
  while ( port != NULL ) 
  {
    if ( port->deviceType == SEABIRD_CTD_19 || 
         port->deviceType == SEABIRD_CTD_19_PLUS ||
         port->deviceType == ENVIROTECH_ECOLAB ||
         port->deviceType == SATLANTIC_ISIS ||
         port->deviceType == SATLANTIC_ISIS_X ) 
    {
      LOGPRINT( LVL_VERB, 
                "getHydroWireDeviceType(): Returning device number %d", 
                port->deviceType );
      return( port->deviceType );
    }
    port = port->nextPort;
  }
  
  LOGPRINT( LVL_VERB, 
            "getHydroWireDeviceType(): Returning: -1" );
  return ( FAILURE );
}


//
// NAME
//   getUSBDeviceContext - Get or open a ftdi context for a USB device 
//
// SYNOPSIS
//   #include "buoy.h"
//
//   struct ftdi_context * getUSBDeviceContext( int deviceType );
//
// DESCRIPTION
//   Return an open USB device context to the device type 
//   listed.  This will check a list of cached open
//   contexts before attempting to open one up itself.
//
// RETURNS
//   The open device context or NULL upon failure.
//
struct ftdi_context * getUSBDeviceContext ( int deviceType )
{
  struct sPort *usbPort;
  struct ftdi_context *ftdic;
  int ftdiRetVal = 0;

  // Say hi
  LOGPRINT( LVL_VERB, "getUSBDeviceContext(): Entered" );

  usbPort = opts.serialPorts;
  while ( usbPort != NULL ) 
  {
    if ( usbPort->deviceType == deviceType )
      break;
    usbPort = usbPort->nextPort;
  }
  if ( usbPort == NULL || usbPort->vendorID == 0 )
  {
    LOGPRINT( LVL_WARN, 
              "getUSBDeviceContext(): Could not find port for "
              "the device type ( %d )!", deviceType );
    return( NULL );
  }
  
  if ( usbPort->ftdiContext != NULL )
    return( usbPort->ftdiContext );

  // Not open yet...open it up!
  ftdic = (struct ftdi_context *)malloc( sizeof( struct ftdi_context ));

  if ( ! ftdic ) 
    return( NULL );

  // initialize the ftdic structure
  if (ftdi_init(ftdic) < 0)
  {
      LOGPRINT( LVL_EMRG, "getUSBDeviceContext(): Could not initialize "
                "FTDI USB interface" );
      return( NULL );
  }

  // Select the first available interface
  //   0=ANY, 1=A, 2=B, 3=C, 4=D
  ftdi_set_interface(ftdic, 0);

  // Open device
  ftdiRetVal = ftdi_usb_open_desc(ftdic, usbPort->vendorID, usbPort->productID,
                                  NULL, usbPort->serialID );
  if (ftdiRetVal < 0)
  {
      LOGPRINT( LVL_EMRG, "getUSBDeviceContext(): Could not open FTDI USB "
                "interface" );
      return( NULL );
  }

  // Set baudrate
  ftdiRetVal = ftdi_set_baudrate(ftdic, usbPort->baud);
  if (ftdiRetVal < 0)
  {
      LOGPRINT( LVL_EMRG, "getUSBDeviceContext(): Could not set baudrate "
                "on FTDI USB interface." );
      return( NULL );
  }

  usbPort->ftdiContext = ftdic;

  return( ftdic );
}
 

//
// NAME
//   getDeviceFileDescriptor - Get or open a file descriptor for device type
//
// SYNOPSIS
//   #include "buoy.h"
//
//   int getDeviceFileDescriptor( int deviceType );
//
// DESCRIPTION
//   Return an open file descriptor to the device type 
//   listed.  This will check a list of cached open
//   file descriptors before attempting to open one
//   up itself.
//
// RETURNS
//   The open file descriptor or -1 upon failure.
//
int getDeviceFileDescriptor ( int deviceType )
{
  struct sPort *port;
  int fd, r;

  // Say hi
  LOGPRINT( LVL_VERB, "getDeviceFileDescriptor(): Entered" );

  port = opts.serialPorts;
  while ( port != NULL ) 
  {
    if ( port->deviceType == deviceType )
      break;
    port = port->nextPort;
  }
  if ( port == NULL || port->tty == NULL )
  {
    LOGPRINT( LVL_WARN, 
              "getDeviceFileDescriptor(): Could not find port for "
              "the device type ( %d )!", deviceType );
    return( FAILURE );
  }
  
  if ( port->fileDescriptor >= 0 )
    return( port->fileDescriptor );

  //
  // Not already open...go ahead and open it
  //
  LOGPRINT( LVL_DEBG, "getDeviceFileDescriptor(): Opening %s: %s", 
            port->description, port->tty );
  fd = open( port->tty, O_RDWR | O_NONBLOCK);
  if ( fd < 0 )
  {
    LOGPRINT( LVL_WARN, "getDeviceFileDescriptor(): Cannot open %s: %s", 
              port->tty, strerror(errno));
    return( FAILURE );
  }
  r = term_set( fd,
                1,               /* raw mode. */
                port->baud,      /* baud rate. */
                port->parity,    /* parity. */
                port->dataBits,  /* data bits. */
                port->flow,      /* flow control. */
                1,               /* local or modem */
                1 );             /* hup-on-close. */
  if ( r < 0 )
  {
    LOGPRINT( LVL_WARN, 
              "getDeviceFileDescriptor(): Failed to add device %s: %s",
              port->tty, term_strerror(term_errno, errno));
    close( fd );
    return( FAILURE );
  }
  r = term_apply(fd);
  if ( r < 0 )
  {
    LOGPRINT( LVL_WARN, 
              "getDeviceFileDescriptor(): Failed to config device %s: %s",
              port->tty, term_strerror(term_errno, errno));
    close( fd );
    return( -1 );
  }
  port->fileDescriptor = fd;

  // Say goodbye
  LOGPRINT( LVL_VERB, "getDeviceFileDescriptor(): Returning: %d", fd );
  return ( fd );
 
}


//
// NAME
//   closeDeviceFileDescriptor - Close cached file descriptors to device
//
// SYNOPSIS
//   #include "buoy.h"
//
//   int closeDeviceFileDescriptor( int deviceType );
//
// DESCRIPTION
//   This is a helper function which will search the cache of
//   open file descriptors for a particular device type and attempt
//   to close them.
// 
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int closeDeviceFileDescriptor ( int deviceType )
{
  struct sPort *port;

  // Say hi
  LOGPRINT( LVL_VERB, "closeDeviceFileDescriptor(): Entered" );

  port = opts.serialPorts;
  while ( port != NULL ) 
  {
    if ( port->deviceType == deviceType )
      break;
    port = port->nextPort;
  }
  if ( port == NULL )
  {
    LOGPRINT( LVL_WARN, 
              "closeDeviceFileDescriptor(): Could not find port for "
              "the device type ( %d )!", deviceType );
    return( FAILURE );
  }
  
  if ( port->fileDescriptor >= 0 )
  {
    term_erase( port->fileDescriptor );
    close( port->fileDescriptor );
    port->fileDescriptor = -1;
  }

  // Say goodbye
  LOGPRINT( LVL_VERB, "closeDeviceFileDescriptor(): Returning: 1" );
  return( SUCCESS );
 
}


