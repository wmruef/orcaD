/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * orcad.h : Header for the main ORCA daemon
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
#ifndef _ORCAD_H
#define _ORCAD_H

#include <sys/types.h>
#include <unistd.h>
#include <ftdi.h>

#include "term.h"

#define CASTIDXFILE "castIndex.txt"
#define BOUY_LATITUDE 47.402523

/* serialDeviceTypes Enumeration
 *
 * Devices one might want to attach to the control
 * computer's serial ports or USB ports.
 *
 */ 
enum serialDeviceTypes {
  SEABIRD_CTD_19,              
  SEABIRD_CTD_19_PLUS,
  AGO_METER_WHEEL_COUNTER,
  DAVIS_WEATHER_STATION,
  ENVIROTECH_ECOLAB,
  SATLANTIC_ISIS,
  SATLANTIC_ISIS_X,
  CELL_MODEM,
  AQUADOPP,
  ARDUINO_METER_WHEEL_COUNTER,
  AIRMAR_PB100,
  AIRMAR_PB200,
  GILLMETPAK,
  RMYOUNGWIND,
  S9VAISALA, 
  SEAFETPH
};

/*
 * Phases of a cast 
 */
enum castDirection {
  UP,
  DOWN,
  BOTH
};
 
/*
 * Serial/USB port characteristics
 */
struct sPort { 
  enum serialDeviceTypes deviceType;
  char *description;
  int  fileDescriptor;
  struct ftdi_context *ftdiContext;
  int vendorID;
  int productID;
  char *serialID;
  char *tty;
  int  baud;
  int  stopBits;
  int  dataBits;
  enum flowcntrl_e flow;
  enum parity_e parity;
  struct sPort *nextPort;
};

/*
 * Mission characteristics
 */
struct mission {
  char *name;
  char startMinsList[60];
  char startHoursList[24];
  char startDaysOfMonthList[32];
  char startMonthsList[12];
  char startDaysOfWeekList[7];  // 0-6 begining on sunday
  pid_t cl_Pid;    /* Running PID, 0, or ready to run -1  */
  float cycles;
  int equilibrationTime;
  int *depths;
  int numDepths;
  int auxSampleDirection;  /* See castDirection enumeration */
  struct mission *nextMission;
};

struct optionsStruct {
  int debugLevel;
  int minDepth;
  int maxDepth;
  int parkingDepth;
  int inCritical;
  int minTimeBetweenSamples;      // Seconds between instrument samples 
  int samplesBeforeMETUpdate;     // # of samples before a metFile update
  int metUpdatesBeforeInstUpdate; // # of metFile updates before instWeather update
  int weatherArchiveDownloadPeriod; // Minutes before metFile is archived
  int auxiliarySamplePeriod;
  int auxiliaryArchiveDownloadPeriod;
  long lastCastNum;
  char isDaemon;
  char *dataFilePrefix;
  char *configFileName;
  char *weatherDataPrefix;
  char *weatherStatusFilename;
  char *auxiliaryDataPrefix;
  char dataDirName[FILEPATHMAX];
  char dataSubDirName[FILEPATHMAX];
  struct sPort *serialPorts;
  struct mission *missions;
  double ctdCalPA1;
  double ctdCalPA2;
  double ctdCalPA3;
  double solarCalibrationConstant;
  double solarMillivoltResistance;
  double solarADMultiplier;
  double meterwheelCFactor;
  double compassDeclination;
} opts;


int initialize();
int runJobs( struct mission *mPtr );
int testJobs(time_t t1, time_t t2, struct mission *mPtr);
void cleanup_TERM();
void cleanup_QUIT();
void cleanup_INT();
void cleanup_HUP();
void cleanup_USR1();
void cleanup(int passed_signal);


#endif
