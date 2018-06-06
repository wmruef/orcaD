/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * auxiliaryd.h : Header for the main ORCA daemon
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
 *
 */
#ifndef _AUXILIARYD_H
#define _AUXILIARYD_H

#include <sys/types.h>
#include <unistd.h>
#include "term.h"

int initialize();
void wsSEAFET();
void processCommandLine(int argc, char *argv[] );
void cleanup_TERM();
void cleanup_QUIT();
void cleanup_INT();
void cleanup_HUP();
void cleanup_USR1();
void cleanup(int passed_signal);

#endif
