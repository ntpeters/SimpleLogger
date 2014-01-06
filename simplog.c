/*
     A very basic logger for output of messages at various logging levels
     with date/time stamp to standard out/err and a defined log file.

     Author: Nate Peterson
     Created: June 2013
     Last Updated: Jan 2014
*/

#include "SimpLogConfig.h"

#ifdef BETTER_BACKTRACE
    #include <execinfo.h>
    #include "backtrace-symbols.h"
#else
    #include <execinfo.h>
#endif

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>




#include "simplog.h"

// Used for printing from within the logger. Prints if debug level is SIMPLOG_DEBUG or higher
#define SIMPLOG_LOGGER  4
// Used for printing stack trace. Prints if debug level is SIMPLOG_DEBUG or higher
#define SIMPLOG_TRACE   5

// Define colors for printing to terminal
#define COL_NORM    "\x1B[0m"   // Normal
#define COL_FATAL   "\x1B[31m"  // Red
#define COL_ERROR   "\x1B[91m"  // Light Red
#define COL_INFO    "\x1B[37m"  // White
#define COL_WARN    "\x1B[33m"  // Yellow
#define COL_DEBUG   "\x1B[94m"  // Light Blue
#define COL_VERBOSE "\x1B[36m"  // Cyan
#define COL_LOGGER  "\x1B[90m"  // Dark Grey
#define COL_TRACE   "\x1B[95m"  // Light Magenta

// Logger settings constants
static int          dbgLevel    = SIMPLOG_DEBUG;        // Default Logging level
static const char*  logFile     = "default.log";    // Default log file name
static bool         silentMode  = false;             // Default silent mode setting

// Private function prototypes
static char* getDateString();

/*
    Writes output to defined logfile and standard out/err with
    date/time stamp and associated log level.
    
    Can take formatted string like printf, with a variable sized list of
    arguments.
     
    Prints errno when appropriate.

    Always adds new line to output.

    Logging Levels:
    -2 : Fatal          - A fatal error has occured: program will exit immediately
    -1 : Error          - An error has occured: program may not exit
    0  : Info           - Nessessary information regarding program operation
    1  : Warnings       - Any circumstance that may not affect normal operation
    2  : Debug          - Standard debug messages (default)
    3  : Debug-Verbose  - All debug messages

    Input:
    int loglvl      - The desired output logging level.  See above table for levels.
    const char* str - The message to be output. This is a format string.
    ...             - Variable length list of arguments to be used with the format string (optional).
*/
void writeLog( int loglvl, const char* str, ... ) {
    // Prepare variable length args list
    va_list args;
    va_start( args, str );

    // No way to determine size of list
    // This will hold a stacktrace of 15 lines plus a message
    int max_va_list_size = 4146;

    // Allocate args string variable
    char* va_msg = (char*)malloc( strlen( str ) + max_va_list_size );

    // Construct final args string
    int va_string_size = vsnprintf( va_msg, strlen( str ) + max_va_list_size, str, args );

    // Open the log file
    int log = open( logFile, O_CREAT | O_APPEND | O_RDWR, 0664 );

    // Get current date/time
    char* date = getDateString();

    // Allocate message variable
    int msgSize = strlen ( date ) + strlen( va_msg ) + strlen( strerror( errno ) ) + 50;  // 10 char buffer to prevent overflow
    char* msg = (char*)malloc( msgSize );

    // Used to hold the current printing color
    char outColor[6];
    memset( outColor, '\0', 6 );
    // Set default color to 'Normal'
    strncpy( outColor, COL_NORM, strlen( COL_NORM ) );

    // Prepare message based on logging level and debug level
    if( loglvl < SIMPLOG_INFO ){
        if( loglvl == SIMPLOG_FATAL ) {
            strncpy( outColor, COL_FATAL, strlen( COL_FATAL ) );
            sprintf( msg, "%s\tFATAL : ", date );   // -2: Fatal
        } else if( loglvl == SIMPLOG_ERROR ) {
            strncpy( outColor, COL_ERROR, strlen( COL_ERROR ) );
            sprintf( msg, "%s\tERROR : ", date );   // -1: Error
        }

        // Append args string to output message
        sprintf( msg + strlen( msg ), "%s\n", va_msg );

        // If errno is anything other than "Success", write it to the log.
        if( errno ) {
            // Used to ensure errno output is aligned correctly
            char dateLengthSpacing[ strlen( date ) + 1 ];
            memset( dateLengthSpacing, ' ', strlen( date ) +1 );
            sprintf( msg + strlen( msg), "%s\terrno : %s\n", dateLengthSpacing, strerror( errno ) );
        }
        // Write message to log
        write( log, msg, strlen( msg ) );
        // Write message to standard error too
        if( !silentMode ) {
            write( STDERR_FILENO, outColor, strlen( outColor ) );
            write( STDERR_FILENO, msg, strlen( msg ) );
        }
    } else {
        // Used to check if a valid combination of log level and debug level exists
        bool valid = true;

        // Check loglvl/dbgLevel and add appropriate name to message
        if( loglvl == SIMPLOG_INFO ) {
            strncpy( outColor, COL_INFO, strlen( COL_INFO ) );
            sprintf( msg, "%s\tINFO  : ", date );      // 0: Info
        } else if( loglvl == SIMPLOG_WARN && dbgLevel >= SIMPLOG_WARN ) {
            strncpy( outColor, COL_WARN, strlen( COL_WARN ) );
            sprintf( msg, "%s\tWARN  : ", date );      // 1: Warning
        } else if( loglvl == SIMPLOG_DEBUG && dbgLevel >= SIMPLOG_DEBUG ) {
            strncpy( outColor, COL_DEBUG, strlen( COL_DEBUG ) );
            sprintf( msg, "%s\tDEBUG : ", date );      // 2: Debug
        } else if( loglvl == SIMPLOG_VERBOSE && dbgLevel >= SIMPLOG_VERBOSE ) {
            strncpy( outColor, COL_VERBOSE, strlen( COL_VERBOSE ) );
            sprintf( msg, "%s\tDEBUG : ", date );      // 3: Verbose
        } else if( loglvl == SIMPLOG_LOGGER && dbgLevel >= SIMPLOG_DEBUG ) {
            strncpy( outColor, COL_LOGGER, strlen( COL_LOGGER ) );
            sprintf( msg, "%s\tLOG   : ", date );
        } else if( loglvl == SIMPLOG_TRACE && dbgLevel >= SIMPLOG_DEBUG ) {
            strncpy( outColor, COL_TRACE, strlen( COL_TRACE ) );
            sprintf( msg, "%s\tTRACE : ", date );
        } else {
            // Don't print anything
            valid = false;
        }

        // Only print if there is a valid match of log level and debug level
        if( valid ) {
            // Append args string to output message
            sprintf( msg + strlen( msg ), "%s\n", va_msg );

            // Write message to log
            write( log, msg, strlen( msg ) );
            // Write message to standard out too
            if( !silentMode ) {
                write( STDOUT_FILENO, outColor, strlen( outColor ) );
                write( STDOUT_FILENO, msg, strlen( msg ) );
            }
        }
    }

    // Reset terminal colors to normal
    write( STDOUT_FILENO, COL_NORM, strlen( COL_NORM ) );
    write( STDERR_FILENO, COL_NORM, strlen( COL_NORM ) );

    // free args list
    va_end( args );

    // close file
    close( log );

    // free other variables
    free( date );
    free( msg );
    free( va_msg );

    // Check if the output was truncated
    if( va_string_size > ( strlen( str ) + max_va_list_size ) ) {
        // get how many bytes the output was truncated by
        int truncated_size = va_string_size - ( strlen( str ) + max_va_list_size );
        // output message notifying the user of truncation and amount
        writeLog( SIMPLOG_LOGGER, "Previous message truncated by %d bytes to fit into buffer", truncated_size );
    }
}

/*
    Prints the stacktrace to logs for the current location in the program.
    Most recent calls appear first.

    Set to a max of 15 lines of the stacktrace for output.
*/
void writeStackTrace() {
    // max lines in backtrace
    static const int max_backtrace_size = 15;

    // holds addresses for backtrace functions
    void* backtrace_addresses[max_backtrace_size];
    // size of backtrace
    size_t backtrace_size = backtrace( backtrace_addresses, max_backtrace_size );

    // string descriptions of each backtrace address
    char** backtrace_strings = backtrace_symbols( backtrace_addresses, backtrace_size );
    // Clear errno
    // It is possible errno is set to a value we don't care about by 'backtrace_symbols'
    errno = 0;

    // max size for the message, assuming individual strings with max of 255 bytes
    int max_message_size = sizeof( backtrace_strings ) * 255;

    // output message to be composed
    char* message = ( char* )malloc( max_message_size );

    // used to ensure consistent alignment between terminal and log file
    char indentedLineSpacing[32];
    memset( indentedLineSpacing, ' ', 31 );
    indentedLineSpacing[30] = '\t';
    //indentedLineSpacing[31] = '\0';

    // Add initial message to the message variable
    sprintf( message, "StackTrace - Most recent calls appear first:\n%s", indentedLineSpacing );
    int initialSize = strlen( message );

    // intermittent offset during message construction
    int offset = initialSize;
    // contstructing the message. starting at index 1 to omit this call
    for( int i = 1; i < backtrace_size; i++ ) {
        // length of the current string
        int string_length = strlen( backtrace_strings[i] );

        // ensure the message buffer is not overflowed
        if( ( offset + string_length - initialSize ) > max_message_size ) {
            // add notice of truncation to the message if there is room
            if( offset + 22 < max_message_size ) {
                strncpy( message + offset, " [backtrace truncated]", 22 );
            }
            // break out of construction prematurely to prevent overflow
            break;
        }

        // copy the current string into the message
        strncpy( message + offset, backtrace_strings[i], string_length );
        offset += string_length;

        // don't add newline and padding to last trace
        if( i < ( backtrace_size - 1 ) ) {
            // add newline and tabs for proper output alignment
            message[offset] = '\n';
            offset += 1;
            strncpy( message + offset, indentedLineSpacing, strlen( indentedLineSpacing ) );
            offset += strlen( indentedLineSpacing );
        }
    }

    // if the buffer has been filled, ensure it is null terminated
    if( offset >= max_message_size - 1 ) {
            // add null byte to the end of the message
            message[offset] = '\0';
    }

    // write the final message to the logs
    writeLog( SIMPLOG_TRACE, "%s", message );

    // free message and backtrace variables
    free( message );
    free( backtrace_strings );
}

/*
    Sets the desired debug level for writing logs.

    Debug Levels:
    0  : Info           - Nessessary information regarding program operation
    1  : Warnings       - Any circumstance that may not affect normal operation
    2  : Debug          - Standard debug messages (default)
    3  : Debug-Verbose  - All debug messages

    Input:
    int level - desired debug level
*/
void setLogDebugLevel( int level ) {
    // Check if the provided debug level is valid, else print error message
    if( level >= SIMPLOG_INFO && level <= SIMPLOG_VERBOSE ) {
         dbgLevel = level;
         writeLog( SIMPLOG_LOGGER, "Debug level set to %d", level );
    } else {
        char* error = (char*)malloc(500);
        sprintf( &error[ strlen( error ) ], "Invalid debug level of '%d'. Setting to default value of '%d'\n", level, SIMPLOG_DEBUG );
        sprintf( &error[ strlen( error ) ], "\t\t\t\tValid Debug Levels:\n");
        sprintf( &error[ strlen( error ) ], "\t\t\t\t0  : Info\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t1  : Warnings\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t2  : Debug\n" );
        sprintf( &error[ strlen( error ) ], "\t\t\t\t3  : Debug-Verbose" );

        writeLog( SIMPLOG_LOGGER, error );
        free( error );
    }
}

/*
    Sets the filename for log output.

    Input:
    const char* file - desired log output file
*/
void setLogFile( const char* file ) {
    logFile = file;
    writeLog( SIMPLOG_LOGGER, "Log file set to '%s'", logFile );
}

/*
    Enables/Disables silent mode.
    When silent mode is enabled, no output will be written to standard out.
    Log output will continue normally.

    Input:
    bool silent - Desired state of silent mode: false = Disabled (default), true = Enabled
*/
void setLogSilentMode( bool silent ) {
    silentMode = silent;
    writeLog( SIMPLOG_LOGGER, "Silent mode %s", silent ? "enabled" : "disabled" );
}

/*
    Flushes the contents of the logfile by deleting it and recreating
    a new empty logfile of the same name.
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
        // Print error message if silent mode is not enabled
        if( !silentMode ) {
            printf( "%s\tLOG   : Logfile '%s' does not exist. It will be created now.\n", getDateString(), logFile );
            fflush( stdout ); 
        }
    }

    // Create new empty log file
    int log = open( logFile, O_CREAT | O_RDWR, 0664 );
    close( log );

    writeLog( SIMPLOG_LOGGER, "Log file '%s' cleared", logFile );
}

/*
    Gets the current date/time and returns it as a string of the form:
    [yyyy-mm-dd hh:mm:ss]

    Returned char pointer must be freed.
*/
static char* getDateString() {
    // Initialize and get current time
    time_t t = time( NULL );
    struct tm *timeinfo = localtime( &t );

    // Allocate space for date string
    char* date = (char*)malloc( 100 );

    // Get each component of time struct that we care about
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
}

// Put all public functions into their own "namespace"
simplog_namespace const simplog = { writeLog, writeStackTrace, setLogDebugLevel, setLogFile, setLogSilentMode, flushLog };
