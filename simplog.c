/*
     A very basic logger for output of messages at various logging levels
     with date/time stamp to standard out and a defined log file.

     Author: Nate Peterson
     Created: June 2013
     Last Updated: Oct 2013
*/

#include "simplog.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>

// Logger settings constants
static int          dbgLevel    = LOG_VERBOSE;
static const char*  logFile     = "default.log";
static bool         silentMode  = false;

// Private function prototypes
static char* getDateString();

/*
    Writes output to defined logfile and standard out with
    date/time stamp and associated log level.
    
    Can take formatted string like printf, with a variable sized list of
    arguments.
     
    Prints errno when appropriate.

    Always adds new line to output.

    Logging Levels:
    -2 : Fatal          - A fatal error has occured: program will exit immediately
    -1 : Error          - An error has occured: program will typically not exit
    0  : Info           - Nessessary information regarding program operation
    1  : Warnings       - Any circumstance that may not affect normal operation
    2  : Debug          - Standard debug messages
    3  : Debug-Verbose  - All debug messages

    Input:
    int loglvl - The desired output logging level.  See above table for levels.
    char* str  - The message to be output. This is a format string.
    ...        - Variable length list of arguments to be used with the format string.
*/
void writeLog( int loglvl, const char* str, ... ) {
    // Open the log file
    int log = open( logFile, O_CREAT | O_APPEND | O_RDWR, 0664 );

    // Get current date/time
    char* date = getDateString();

    // Prepare variable length args list
    va_list args;
    va_start( args, str );
    int max_va_list_size = 250;  // No way to determine size of list.  250 should be a good ceiling.

    // Allocate message variable
    int msgSize = strlen( str ) + strlen ( date ) + strlen( strerror( errno ) ) + 10;  // 10 char buffer to prevent overflow
    char* msg = (char*)malloc( msgSize + max_va_list_size );

    if( loglvl < LOG_INFO ){
        if( loglvl == LOG_FATAL ) {
               sprintf( msg, "%s\tFATAL : ", date );   // -2: Fatal
        } else if( loglvl == LOG_ERROR ) {
            sprintf( msg, "%s\tERROR : ", date );      // -1: Error
        }

        vsprintf( msg + strlen( msg ), str, args );
        sprintf( msg + strlen( msg ), "\n" );

        // If errno is anything other than "Success", write it to the log.
        if( errno ) {
            sprintf( msg + strlen( strerror( errno ) ), "%s\n", strerror( errno ) );
        }
        // Write message to log
        write( log, msg, strlen( msg ) );
        // Write message to standard error too
        if( !silentMode ) {
            write( STDERR_FILENO, msg, strlen( msg ) );
        }
    } else {
        bool valid = true;

        if( loglvl == LOG_INFO ) {
            sprintf( msg, "%s\tINFO  : ", date );      // 0: Info
        } else if( loglvl == LOG_WARN && dbgLevel >= LOG_WARN ) {
            sprintf( msg, "%s\tWARN  : ", date );      // 1: Warning
        } else if( loglvl == LOG_DEBUG && dbgLevel >= LOG_DEBUG ) {
            sprintf( msg, "%s\tDEBUG : ", date );      // 2: Debug
        } else if( loglvl == LOG_VERBOSE && dbgLevel >= LOG_VERBOSE ) {
            sprintf( msg, "%s\tDEBUG : ", date );      // 3: Verbose
        } else {
            // Don't print anything
            valid = false;
        }

        // Only print if there is a valid match of log level and debug level
        if( valid ) {
            vsprintf( msg + strlen( msg ), str, args );
            sprintf( msg + strlen( msg ), "\n" );

            // Write message to log
            write( log, msg, strlen( msg ) );
            // Write message to standard out too
            if( !silentMode ) {
                write( STDOUT_FILENO, msg, strlen( msg ) );
            }
        }
    }

    // free args list
    va_end( args );

    close( log );
    free( date );
    free( msg );
}

/*
    Sets the desired debug level for writing logs.

    Debug Levels:
    0  : Info           - Nessessary information regarding program operation
    1  : Warnings       - Any circumstance that may not affect normal operation
    2  : Debug          - Standard debug messages
    3  : Debug-Verbose  - All debug messages

    Input:
    int level - desired debug level
*/
void setLogDebugLevel( int level ) {
    if( level >= LOG_INFO && level <= LOG_VERBOSE ) {
         dbgLevel = level;
    } else {
        char* error = (char*)malloc(500);
        sprintf( &error[ strlen( error ) ], "Invalid debug level of '%d'. Setting to default value of '%d'\n", level, dbgLevel );
        sprintf( &error[ strlen( error ) ], "\t\t\t\tValid Debug Levels:\n");
        sprintf( &error[ strlen( error ) ], "\t\t\t\t0  : Info\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t1  : Warnings\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t2  : Debug\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t3  : Debug-Verbose" );

        writeLog( LOG_ERROR, error );
        free( error );
    }
}

/*
    Sets the filename for log output.

    Input:
    char* file - desired log output file
*/
void setLogFile( const char* file ) {
    logFile = file;
}

/*
    Enables/Disables silent mode.
    When silent mode is enabled, no output will be written to standard out.
    Log output will continue normally.

    Input:
    int silent - Desired state of silent mode: 0 = Disabled, 1 = Enabled (default)
*/
void setLogSilentMode( bool silent ) {
    silentMode = silent;
    writeLog( LOG_DEBUG, "Silent mode enabled" );
}

/*
    Flushes the contents of the logfile by deleting it and recreating
    a new empty logfile.
*/
void flushLog() {
    // Check if file exists
    if( !access( logFile, F_OK ) ) {
        // Remove the old log file
        int err = remove( logFile );
        if( err ) {
            perror( "ERROR: Unable to flush logfile!" );
            exit( -1 );
        }
    } else {
        printf( "%s\tERROR : Logfile '%s' does not exist. It will be created now.\n", getDateString(), logFile );
        fflush(stdout);
    }

    // Create new empty log file
    int log = open( logFile, O_CREAT | O_RDWR, 0664 );
    close( log );
}

/*
    Gets the current date/time and returns it as a string of the form:
    [yyyy-mm-dd hh:mm:ss]
*/
static char* getDateString() {
    time_t t = time ( NULL );
    struct tm *timeinfo = localtime( &t );
    char* date = (char*)malloc( 100 );

    int year    = timeinfo->tm_year + 1900;
    int month   = timeinfo->tm_mon + 1;
    int day     = timeinfo->tm_mday;
    int hour    = timeinfo->tm_hour;
    int min     = timeinfo->tm_min;
    int sec     = timeinfo->tm_sec;

    sprintf( date, "[%d-", year );

    // Add zero for single digit values
    if( month < 10 ) {
        sprintf( date + strlen( date ), "0%d-", month );
    } else {
        sprintf( date + strlen( date ), "%d-", month );
    }
    if( day < 10 ) {
        sprintf( date + strlen( date ), "0%d ", day );
    } else {
        sprintf( date + strlen( date ), "%d ", day );
    }
    if( hour < 10 ) {
        sprintf( date + strlen( date ), "0%d:", hour );
    } else {
        sprintf( date + strlen( date ), "%d:", hour );
    }
    if( min < 10 ) {
        sprintf( date + strlen( date ), "0%d:", min );
    } else {
        sprintf( date + strlen( date ), "%d:", min );
    }
    if( sec < 10 ) {
        sprintf( date + strlen( date ), "0%d]", sec );
    } else {
        sprintf( date + strlen( date ), "%d]", sec );
    }

    return date;

    // TODO: Simplify
}
