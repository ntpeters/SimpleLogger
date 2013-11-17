/*
     A very basic logger for output of messages at various logging levels
     with date/time stamp to standard out and a defined log file.

     Author: Nate Peterson
     Created: June 2013
     Last Updated: Nov 2013
*/

#ifndef SIMPLOG_H
#define SIMPLOG_H

// Define logging levels
#define LOG_FATAL    -2    // A fatal error has occured: program will exit immediately
#define LOG_ERROR    -1    // An error has occured: program may not exit
#define LOG_INFO     0     // Nessessary information regarding program operation
#define LOG_WARN     1     // Any circumstance that may not affect normal operation
#define LOG_DEBUG    2     // Standard debug messages
#define LOG_VERBOSE  3     // All debug messages

#include <stdbool.h>

// Public functions
void writeLog( int loglvl, const char* str, ... );
void setLogDebugLevel( int level );
void setLogFile( const char* file );
void setLogSilentMode( bool silent );
void flushLog();

#endif
