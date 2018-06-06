/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * parser.h : Header for the config file parsing functions
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
#ifndef _PARSER_H
#define _PARSER_H

int getNextConfigTokens( FILE *fIN, char *buffer, char **name, 
                         char **value, int *linesRead );
int tokenizeConfigLine( char *buffer, char **name, char **value );
int parseConfigFile( char *fileName );
void logOpts( FILE * fd );
char * convArrayToRangeString( char *ary, int arraySize, 
                               const char *const *names );
char *parseCronField(char *ary, int modvalue, int off,
                     const char *const *names, char *ptr);
void fixDayDow(struct mission *misn);

#endif
