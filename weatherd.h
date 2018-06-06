/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * weatherd.h : Header for the main ORCA daemon
 *
 * Created: March 2010
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
#ifndef _WEATHERD_H
#define _WEATHERD_H

#include <sys/types.h>
#include <unistd.h>
#include "term.h"

/* S9SentenceTypes Enumeration
 *
 * Sentence types we care to keep.  This
 * list should be kept in sync with the
 * S9SentenceNames array and the
 * numSentenceTypes count which follow.
 *
 */ 
enum S9SentenceTypes {
    CREPORT,
    S9CD,
    S9CRD,
    PORT1,
    PORT1END,
    CREPORTEND
};

static const char *S9SentenceNames[] = {
  "<crep", // "<creport"
  "<S9CD", // "<S9CD"
  "<S9CR", // "<S9CRD"
  "<PORT1", // "<PORT1>"
  "</PORT1", // "</PORT1>"
  "</cre"  // "</creport>"
};

const int numSentenceTypes = 6;


struct s_WMDA {
  float barsBarometricPressure;
  float inchesBarometricPressure;
  float airTemperature;
  float relativeHumidityPct;
  float dewPoint;
  float windDirectionTrue;
  float windDirectionMagnetic;
  float windSpeedKnots;
  float windSpeedMetersPerSec;
};

struct s_GPGGA {
  int   hourUTC;
  int   minUTC;
  int   secUTC;
  float latitude;
  char  latitudeDir;
  float longitude;
  char  longitudeDir;
  int   gpsQuality;
  int   numSatellites;
};

struct s_WIMWD {
  float windDirectionTrue;
  float windDirectionMagnetic;
  float windSpeedKnots;
  float windSpeedMetersSec;
};

struct s_WIMWV {
  float windDirectionCtr;
  char  windReference;
  float windSpeed;
  char  windSpeedUnit;
  char status;
};

struct s_CombinedWeatherData {
  int month;
  int day;
  int year;
  int hour;
  int minute;
  float inchesBarometricPressure;
  float pctHumidity;
  float temperatureCelsius;
  float dewpointCelsius;
  float instWindDirectionTrue;
  float instWindDirectionCenterline;
  float instWindSpeed;
  float instU;
  float instV;
  float maxWindSpeedDirectionCenterline;
  float maxWindSpeed;
  float instSolarRadiation;
  float avgSolarRadiation;
  float latitude;
  float longitude;
  float compassDir;
  int numSamples;
  float INC;
  int MAG;
  float THETA;
  float PHI;
  float TILT;
  float TEMP; 
  int RATE;
  int NS;
  float WM;
  float WP;
  float DM;
  float DX;
  float GU;
  float TS;
  int VA;
  int FS;
  float VI;
};

int initialize();
int wsMonitor();
void clearCombinedWeatherData( struct s_CombinedWeatherData * wData );
void processCommandLine(int argc, char *argv[] );
int S9GetSentenceType( const char *buff );
void cleanup_TERM();
void cleanup_QUIT();
void cleanup_INT();
void cleanup_HUP();
void cleanup_USR1();
void cleanup(int passed_signal);

#endif
