/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * aquadopp.h: Header for Aquadopp functions 
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
#ifndef _AQUADOPP_H
#define _AQUADOPP_H 

//
// The length of the input buffer for Aquadopp communications
//
#define AQUABUFFLEN 256




struct aquadoppFAT {
  // Filename ASCII
  char filenamePrefix[6];
  // Filename suffix number incremented when filename is not unique
  char filenameSeq;
  // LSB bit indicates file wrapping if enabled
  char status;
  // Start address in memory 
  char startAddress[4];
  // End address in memory 
  char stopAddress[4];
}__attribute__ ((packed)); 


//
// Data structures for the "GA" command which returns the
// hardware configuration, the head configuration and the 
// deployment configuration from the instrument.
//
struct hardwareConfig {
  // Sync 0xA5
  char sync;
  // Id 0x05
  char id;
  // Size of structure in words ( 2 byte words )
  char size[2];
  // Instrument type and serial number
  char serialNo[14];
  // Board configuration:
  //   bit 0: Recorder installed (0=no, 1=yes)
  //   bit 1: Compass installed (0=no, 1=yes)
  char config[2];
  // Board frequency [kHz]
  char frequency[2];
  // PIC code version number
  char picVersion[2];
  // Hardware revision
  char hwRevision[2];
  // Recorder size (*65536 bytes)
  char recSize[2];
  // Status: bit 0: Velocity range (0=normal, 1=high)
  char status[2];
  // Spare
  char spare_5[12];
  // Firmware version
  char fwVersion[4];
  // Checksum = 0xB58C + sum of all bytes in structure
  char checksum[2];
}__attribute__ ((packed)); 


struct headConfig {
  // 0xA5 (hex)
  char sync;
  // 0x04 (hex)
  char id;
  // Size of structure in number of words (1 word = 2 bytes)
  char size[2];
  // Head configuration:
  //   bit 0: Pressure sensor (0=no, 1=yes)
  //   bit 1: Magnetometer sensor (0=no, 1=yes)
  //   bit 2: Tilt sensor (0=no, 1=yes)
  //   bit 3: Tilt sensor mounting (0=up, 1=down)
  char config[2];
  // Head frequency (kHz)
  char frequency[2];
  // Head type
  char type[2];
  // Head serial number
  char serialNo[12];
  // System data
  char system[176];
  // Spare
  char spare[22];
  // Number of beams
  char nBeams[2];
  // 0xB58C(hex) + sum of all bytes in structure
  char checksum[2];
}__attribute__ ((packed)); 

struct userConfig {
  // Sync 0xa5 (hex)
  char sync;
  // Id 0x00 (hex)
  char id;
  // Size of structure in number of words (1 word = 2 bytes)
  char size[2];
  // Transmit pulse length (counts)
  char t1[2];
  // Blanking distance (counts)
  char t2[2];
  // Receive length (counts)
  char t3[2];
  // Time between pings (counts)
  char t4[2];
  // Time between burst sequences (counts)
  char t5[2];
  // Number of beam sequences per burst
  char nPings[2];
  // Average interval in seconds
  char avgInterval[2];
  // Number of beams
  char nBeams[2];
  // Timing controller mode
  //   bit 1: profile (0=single, 1=continuous)
  //   bit 2: mode (0=burst, 1=continuous)
  //   bit 5: power level (0=1, 1=2, 0=3, 1=4)
  //   bit 6: power level (0 0 1 1 )
  //   bit 7: synchout position (0=middle of sample, 1=end of sample (Vector))
  //   bit 8: sample on synch (0=disabled,1=enabled, rising edge)
  //   bit 9: start on synch (0=disabled,1=enabled, rising edge)
  char timCtrlReg[2];
  // Power control register
  //   bit 5: power level (0=1, 1 =2, 0=3, 1=4)
  //   bit 6: power level (0 0 1 1 )
  char pwrCtrlReg[2];
  // Not used
  char a1[2];
  // Not used
  char b0[2];
  // Not used
  char b1[2];
  // Compass update rate
  char compassUpdRate[2];
  // Coordinate system (0=ENU, 1=XYZ, 2=BEAM)
  char coordSystem[2];
  // Number of cells
  char nBins[2];
  // Cell size
  char binLength[2];
  // Measurement interval
  char measInterval[2];
  // Recorder deployment name
  char deployName[6];
  // Recorder wrap mode (0=NO WRAP, 1=WRAP WHEN FULL)
  char wrapMode[2];
  // Deployment start time
  char clockDeploy[6];
  // Number of seconds between diagnostics measurements
  char diagInterval[4];
  // Mode:
  //   bit 0: use user specified sound speed (0=no, 1=yes)
  //   bit 1: diagnostics/wave mode 0=disable, 1=enable)
  //   bit 2: analog output mode (0=disable, 1=enable)
  //   bit 3: output format (0=Vector, 1=ADV)
  //   bit 4: scaling (0=1 mm, 1=0.1 mm)
  //   bit 5: serial output (0=disable, 1=enable)
  //   bit 6: reserved EasyQ
  //   bit 7: stage (0=disable, 1=enable)
  //   bit 8: output power for analog input (0=disable, 1=enable)
  char mode_1[2];
  // User input sound speed adjustment factor
  char adjSoundSpeed[2];
  // Samples (AI if EasyQ) in diagnostics mode
  char nSampDiag[2];
  // # beams / cell number to measure in diagnostics mode
  char nBeamsCellDiag[2];
  // # pings in diagnostics/wave mode
  char nPingsDiag[2];
  // Mode test:
  //   bit 0: correct using DSP filter (0=no filter, 1=filter)
  //   bit 1: filter data output (0=total corrected velocity,
  //                              1=only correction part)
  char modeTest[2];
  // Analog input address
  char anaInAddr[2];
  // Software version
  char swVersion[2];
  // Spare
  char spare_1[2];
  // Velocity adjustment table
  char velAdjTable[180];
  // File comments
  char comments[180];
  // Wave measurement mode
  //   bit 0: data rate (0=1 Hz, 1=2 Hz)
  //   bit 1: wave cell position (0=fixed, 1=dynamic)
  //   bit 2: type of dynamic position (0=pct of mean pressure, 1=pct of min re)
  char mode_2[2];
  // Percentage for wave cell positioning (=32767×#%/100) (# means number of)
  char dynPercPos[2];
  // Wave transmit pulse
  char _T1[2];
  // Fixed wave blanking distance (counts)
  char _T2[2];
  // Wave measurement cell size
  char _T3[2];
  // Number of diagnostics/wave samples
  char NSamp[2];
  // Not used
  char _A1[2];
  // Not used
  char _B0[2];
  // Not used
  char _B1[2];
  // Spare
  char spare_2[2];
  // Analog output scale factor (16384=1.0, max=4.0)
  char anaOutScale[2];
  // Correlation threshold for resolving ambiguities
  char corrThresh[2];
  // Spare
  char spare_3[2];
  // Transmit pulse length (counts) second lag
  char tiLag2[2];
  // Spare
  char spare_4[30];
  // Stage match filter constants (EZQ)
  char qualConst[16];
  // Checksum = 0xB58C(hex) + sum of all bytes in structure
  char checksum[2];
}__attribute__ ((packed)); 

//
// Prototypes
//

//
int displayAquadoppFAT( int aquadoppFD );

//
int clearAquadoppFAT( int aquadoppFD );

//
int downloadAquadoppFiles( int aquadoppFD );

//   initAquadopp - Initialize the Aquadopp for normal operations
int initAquadopp( int aquadoppFD );

//
int syncAquadoppTime( int aquadoppFD );

//   getAquadoppPrompt - Obtain a command prompt from a Aquadopp 
int getAquadoppPrompt( int aquadoppFD );

//   stopLoggingAquadopp - Cease Aquadopp logging
int stopLoggingAquadopp( int aquadoppFD );

//   startLoggingAquadopp - Initiate Aquadopp logging
int startLoggingAquadopp( int aquadoppFD );

//
int displayAquadoppConfig( int aquadoppFD );

//
int littleToBigEndianWord( char * data );

//
char bcdToDec( char bcdVal );

//
char decToBCD( char decVal );

//   getAquadoppTime - Get the date/time from the Aquadopp 
struct tm *getAquadoppTime( int aquadoppFD );

//   setAquadoppTime - Set the date/time on the Aquadopp 
int setAquadoppTime( int aquadoppFD, struct tm *time );

#endif
