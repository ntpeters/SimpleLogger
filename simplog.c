/*
     A very basic logger for output of messages at various logging levels
     with date/time stamp to standard out/err and a defined log file.

     Author: Nate Peterson
     Created: June 2013
     Last Updated: Feb 2014
*/

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <execinfo.h>
#include <stdint.h>

#ifdef __APPLE__
     #include <libproc.h>
#endif

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
static int  dbgLevel        = SIMPLOG_DEBUG;    // Default Logging level
static char logFile[255]    = "default.log";    // Default log file name
static bool silentMode      = false;            // Default silent mode setting
static bool lineWrap        = true;             // Default setting for line wrapping

// Private function prototypes
static char* getDateString();
static char** getPrettyBacktrace( void* addresses[], int array_size );
static void wrapLines( char* msg, int msgSize );

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

    // Calculate total size of the string
    int stringSize = strlen( date ) + strlen( va_msg ) + 10;
    // Calculate space used by line wraps
    int lineWrapSize = ( 32 * ( stringSize / 80 ) );
    // Calculate the total message size
    int msgSize = stringSize + lineWrapSize + strlen( strerror( errno ) ) + 50;  // 50 char buffer to prevent overflow

    // Allocate message variable
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

        // Check if the current message should be wrapped
        if( lineWrap && strlen( msg ) > 80 ) {
            wrapLines( msg, msgSize );
        }

        // If errno is anything other than "Success", write it to the log.
        if( errno ) {
            // Used to ensure errno output is aligned correctly
            char dateLengthSpacing[ strlen( date ) + 1 ];
            memset( dateLengthSpacing, ' ', strlen( date ) + 1 );
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
        // Used to check if we should try wrapping lines or not
        bool wrap  = true;

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
            // Internal logger messages should appear as they are. Don't wrap
            wrap = false;
        } else if( loglvl == SIMPLOG_TRACE && dbgLevel >= SIMPLOG_DEBUG ) {
            strncpy( outColor, COL_TRACE, strlen( COL_TRACE ) );
            sprintf( msg, "%s\tTRACE : ", date );
            // Traces are pre-formatted.  Don't attempt to wrap the lines
            wrap = false;
        } else {
            // Don't print anything
            valid = false;
        }

        // Only print if there is a valid match of log level and debug level
        if( valid ) {
            // Append args string to output message
            sprintf( msg + strlen( msg ), "%s\n", va_msg );

            // Check if the current message should be wrapped
            if( lineWrap && wrap && strlen( msg ) > 80 ) {
                wrapLines( msg, msgSize );
            }

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

    // used to know if pretty backtrace was returned
    bool freePrettyBacktrace = true;

    // string descriptions of each backtrace address
    char** backtrace_strings = getPrettyBacktrace( backtrace_addresses, backtrace_size );
    if( backtrace_strings == NULL ) {
        backtrace_strings = backtrace_symbols( backtrace_addresses, backtrace_size );
        freePrettyBacktrace = false;
    }
    // Clear errno
    // It is possible errno is set to a value we don't care about by 'backtrace_symbols'
    errno = 0;

    // max size for the message, assuming individual strings with max of 255 bytes
    int max_message_size = sizeof( backtrace_strings ) * 255;

    // output message to be composed
    char* message = ( char* )malloc( max_message_size );

    // used to ensure consistent alignment between terminal and log file
    char* indentedLineSpacing = (char*)malloc( 32 );
    memset( indentedLineSpacing, ' ', 32 );
    indentedLineSpacing[30] = '\t';
    indentedLineSpacing[31] = 0;

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

    // ensure it is null terminated
    if( offset >= max_message_size - 1 ) {
            // if the message has been filled or overfilled, truncate
            message[max_message_size - 1] = 0;
    } else {
        // add null byte to the end of the message
        message[offset] = 0;
    }

    // write the final message to the logs
    writeLog( SIMPLOG_TRACE, "%s", message );

    // free message and backtrace variables
    free( message );
    free( indentedLineSpacing );

    // free backtrace strings
    if( freePrettyBacktrace ) {
        for( int i = 0; i < backtrace_size; i++ ) {
            free( backtrace_strings[i] );
        }
    }
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
        // set to default debug level
        dbgLevel = SIMPLOG_DEBUG;

        // used to ensure consistent alignment between terminal and log file
        char* indentedLineSpacing = (char*)malloc( 32 );
        memset( indentedLineSpacing, ' ', 32 );
        indentedLineSpacing[30] = '\t';
        indentedLineSpacing[31] = 0;

        // prepare error message
        char* error = (char*)malloc( 500 );
        sprintf( error, "Invalid debug level of '%d'. Setting to default value of '%d'\n", level, SIMPLOG_DEBUG );
        sprintf( error + strlen( error ), "%sValid Debug Levels:\n", indentedLineSpacing );
        sprintf( error + strlen( error ), "%s0  : Info\n", indentedLineSpacing );
        sprintf( error + strlen( error ), "%s1  : Warnings\n", indentedLineSpacing );
        sprintf( error + strlen( error ), "%s2  : Debug\n", indentedLineSpacing );
        sprintf( error + strlen( error ), "%s3  : Debug-Verbose", indentedLineSpacing );

        writeLog( SIMPLOG_LOGGER, error );
        free( error );
        free( indentedLineSpacing );
    }
}

/*
    Sets the filename for log output.

    Input:
    const char* file - desired log output file
*/
void setLogFile( const char* file ) {
    memset( logFile, 0, 255 );
    strcpy( logFile, file );
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
    Enables/Disables line wrapping.
    When line wrapping is enabled, lines that are over 80 characters will be 
    wrapped multiple times so that each line is below 80 characters.

    Input:
    bool wrap - Desired state of line wrapping: true = Enabled (default), false = Disabled
*/
void setLineWrap( bool wrap ) {
    lineWrap = wrap;
    writeLog( SIMPLOG_LOGGER, "Line wrapping %s", wrap ? "enabled" : "disabled" );
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
            write( STDOUT_FILENO, COL_LOGGER, strlen( COL_LOGGER ) );
            printf( "%s\tLOG   : Logfile '%s' does not exist. It will be created now.\n", getDateString(), logFile );
            write( STDOUT_FILENO, COL_NORM, strlen( COL_NORM ) );
            fflush( stdout );
        }
    }

    // Create new empty log file
    int log = open( logFile, O_CREAT | O_RDWR, 0664 );
    close( log );

    writeLog( SIMPLOG_LOGGER, "Log file '%s' cleared", logFile );
}

/*
    Loads logger configuration settings from the given config file.

    Supported settings are:

    silent  - Enables/disables silent mode (see setLogSilentMode)
    wrap    - Enables/disables line wrapping (see setLineWrap)
    flush   - Determines if the log file should be cleared (see flushLog)
    debug   - Sets the debug level (see setLogDebugLevel)
    logfile - Sets the path to the log file (see setLogFile)

    Input:
    const char* config - Logger config file to parse and load settings from
*/
void loadConfig( const char* config ) {
    // Settings variables (set to default values)
    bool silentSetting      = silentMode;
    bool lineWrapSetting    = lineWrap;
    bool flushSetting       = false;
    int debugLevelSetting   = dbgLevel;
    char logfileSetting[255];
    strcpy( logfileSetting, logFile );

    // Open config file
    int fd = open( config, O_RDONLY );
    if( fd == -1 ) {
        // Write error message
        writeLog( SIMPLOG_LOGGER, "Unable to open config file: '%s'", config );

        // Clean errno so it's not mistakenly printed for other messages
        errno = 0;

        // Exit function early. No log file to read.
        return;
    }

    // Initial buffer size for reading settings
    int SETTINGS_BUFSIZE = 1024;
    // Allocate settings buffer
    char* settingsBuffer = (char*)malloc( SETTINGS_BUFSIZE );
    // Keep track of bytes read
    ssize_t bytesRead = 0;
    // Read in the entire config file for processing
    while( ( bytesRead = read( fd + bytesRead, settingsBuffer, SETTINGS_BUFSIZE ) ) > 0 ) {
        // If we read enough to fill the buffer, realloc it to twice its size
        if( bytesRead == SETTINGS_BUFSIZE ) {
            settingsBuffer = (char*)realloc( (void*)settingsBuffer, SETTINGS_BUFSIZE * 2 );
        }
    }
    // Done reading. Close the file.
    close( fd );

    // Line start, split, and end incices for each line read
    int startL  = 0;
    int splitL  = 0;
    int endL    = 0;
    // Buffers to hold variable and value for each line
    char var[255];
    char val[255];
    // Parse the config file
    for( int i = 0; i < strlen( settingsBuffer ); i++ ) {
        if( settingsBuffer[i] == '=' ) {
            // Save the index to split the current line on
            splitL = i + 1;
        } else if( settingsBuffer[i] == '\n' ) {
            // Save the end index for the curren line
            endL = i;

            // Zero buffers
            memset( var, 0, 255 );
            memset( val, 0, 255 );

            // Copy variable name and value into buffers
            strncpy( var, settingsBuffer + startL, splitL - startL - 1 );
            strncpy( val, settingsBuffer + splitL, endL - splitL );

            // Parse the value if it is a boolean
            bool boolVal;
            if( strcmp( val, "true" ) == 0 ) {
                boolVal = true;
            } else if( strcmp( val, "false" ) == 0 ) {
                boolVal = false;
            }

            // Parse the settings variables
            if( strcmp( var, "silent" ) == 0 ) {
                silentSetting = boolVal;
            } else if( strcmp( var, "flush" ) == 0 ) {
                flushSetting = boolVal;
            } else if( strcmp( var, "wrap" ) == 0 ) {
                lineWrapSetting = boolVal;
            } else if( strcmp( var, "debug" ) == 0 ) {
                debugLevelSetting = atoi( val );
            } else if( strcmp( var, "logfile" ) == 0 ) {
                strcpy( logfileSetting, val );
            }

            // Set the start index for the next line
            startL = i + 1;
        }
    }

    // Apply all settings
    if( silentSetting ) {
        memset( logFile, 0, 255 );
        strcpy( logFile, logfileSetting );
    } else {
        setLogFile( logfileSetting );
    }
    if( flushSetting ) {
        flushLog();
    }
    setLogSilentMode( silentSetting );
    setLineWrap( lineWrapSetting );
    setLogDebugLevel( debugLevelSetting );
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

/*
    Gets a more human readable version of backtrace with function/file names
    and line numbers

    Input:
    void* addresses[]   - array of addresses returned by a call to backtrace()
    int array_size      - size of the addresses array returned by backtrace();

    Returns a a list of strings describing the backtrace addresses. This list
    must be freed by the caller.  On error, NULL is returned.
*/
static char** getPrettyBacktrace( void* addresses[], int array_size ) {
    // Used to return the strings generated from the addresses
    char** backtrace_strings = (char**)malloc( sizeof( char* ) * array_size );
    for( int i = 0; i < array_size; i ++ ) {
        backtrace_strings[i] = (char*)malloc( sizeof( char ) * 255 );
    }

    int max_path_size = 4096;
    int max_command_string_size = max_path_size + 255;

    // Will hold the command to be used (max size of path + 255)
    char command_string[max_command_string_size]
;   // set to the maximum possible path size
    char exe_path[max_path_size];

    // Used to check if an error occured while setting up command
    bool error = false;

    // Check if we are running on Mac OS or not, and select appropriate command
    const char* command;
    #ifdef __APPLE__
        // Check if 'gaddr2line' function is available, if not exit
        if( !system( "which gaddr2line > /dev/null 2>&1" ) ) {
            command = "gaddr2line -Cfispe";
            pid_t pid = getpid();
            int path_length = proc_pidpath( pid, exe_path, sizeof( exe_path ) );
            if( path_length <= 0 ) {
                writeLog( SIMPLOG_LOGGER, "Unable to get execution path. Defaulting to standard backtrace." );
                error = true;
            }
            exe_path[path_length] = 0;
        } else {
            writeLog( SIMPLOG_LOGGER, "Function 'gaddr2line' unavailable. Defaulting to standard backtrace. Please install package 'binutils' for better stacktrace output." );
            error = true;
        }
    #else
        // Check if 'addr2line' function is available, if not exit
        if( !system( "which addr2line > /dev/null 2>&1" ) ) {
            command = "addr2line -Cfispe";
            int path_length = readlink( "/proc/self/exe", exe_path, sizeof( exe_path ) );
            if(  path_length <= 0 ) {
                writeLog( SIMPLOG_LOGGER, "Unable to get execution path. Defaulting to standard backtrace." );
                error = true;
            }
            exe_path[path_length] = 0;
        } else {
            writeLog( SIMPLOG_LOGGER, "Function 'addr2line' unavailable. Defaulting to standard backtrace. Please install package 'binutils' for better stacktrace output." );
            error = true;
        }
    #endif

    // If an error occured, exit now
    if( error ) {
        for( int i = 0; i < array_size; i ++ ) {
            free( backtrace_strings[i] );
        }
        free( backtrace_strings );
        return NULL;
    }

    // Used to check if the addresses were successfully evaluated
    bool address_evaluation_successful = false;

    // Evaluate all addresses
    for( int i = 0; i < array_size; i++ ) {
        // Compose the complete command to execute
        sprintf( command_string, "%s \"%s\" %X 2>/dev/null", command, exe_path, (unsigned int)(uintptr_t)addresses[i] );

        // Execute the command
        FILE* line = popen( command_string, "r" );

        // Error checking for command
        if( line == NULL ) {
            writeLog( SIMPLOG_LOGGER, "Failed to execute command: '%s'. Defaulting to standard backtrace.", command );
            for( int i = 0; i < array_size; i ++ ) {
                free( backtrace_strings[i] );
            }
            free( backtrace_strings );
            return NULL;
        }

        // Read the output into the return string
        if( fgets( backtrace_strings[i] , 255, line ) == NULL ) {
            writeLog( SIMPLOG_LOGGER, "Failed to get pretty backtrace strings. Defaulting to standard backtrace." );
            for( int i = 0; i < array_size; i ++ ) {
                free( backtrace_strings[i] );
            }
            free( backtrace_strings );
            return NULL;
        }

        // Remove newline and set to null bit
        backtrace_strings[i][ strlen( backtrace_strings[i] ) - 1 ] = 0;

        // If any addresses are able to be evaluated, we consider it a success
        if( strcmp( backtrace_strings[i], "??" ) != 0 && strcmp( backtrace_strings[i], "?? ??:0" ) != 0 ) {
            address_evaluation_successful = true;
        }

        // Close the command pipe
        pclose( line );
    }

    // If no addresses were evaluated successfully, we fall back on the standard backtrace
    if( !address_evaluation_successful ) {
        writeLog( SIMPLOG_LOGGER, "Command '%s' failed to evaluate addresses. Defaulting to standard backtrace.", command );
        for( int i = 0; i < array_size; i ++ ) {
            free( backtrace_strings[i] );
        }
        free( backtrace_strings );
        return NULL;
    }

    // Return the final list of backtrace strings
    return backtrace_strings;
}

/*
    Wraps the message into multiple 80 character lines.

    Input:
    char* msg    - The message variable to wrap
    int msgSize  - The maximum message size
 */
static void wrapLines( char* msg, int msgSize ) {
    // used to ensure consistent alignment between terminal and log file
    char* indentedLineSpacing = (char*)malloc( 32 );
    memset( indentedLineSpacing, ' ', 32 );
    indentedLineSpacing[30] = '\t';
    indentedLineSpacing[31] = 0;

    // Temp buffer to hold the newly constructed message
    char tempBuf[msgSize];
    memset( tempBuf, 0, msgSize );

    // Used to keep track of how much is read from message for each line
    int lineFeedSize = 80;

    // copy first 80 characters from message
    strncpy( tempBuf, msg, lineFeedSize );

    // Iterate through the message to wrap all lines
    for( int i = lineFeedSize; i < strlen( msg ); i += lineFeedSize ) {
        // Check if the current line (including whitespace) is over 80 characters
        if( strlen( msg + i ) + strlen( indentedLineSpacing ) > 80 ) {
            // Get the position of the last space in the line
            char* returnPos = strrchr( tempBuf, ' ' );
            // Replace the space with a newline
            returnPos[0] = '\n';

            // Temp buffer to store the text after the space
            char wrapText[255];
            // Copy the text that was after the space into the buffer
            strcpy( wrapText, returnPos + 1 );

            // Write indent spacing for this line, and append the
            // wrapped text from after the space
            sprintf( returnPos + 1, "%s%s", indentedLineSpacing, wrapText );

            // Calculate how much to copy in from the message
            lineFeedSize = ( strlen( msg + i ) < 80 ? strlen( msg + i ) : 80 )  - strlen( indentedLineSpacing ) - strlen( wrapText );
            // Copy from the message
            strncpy( returnPos + strlen( returnPos ), msg + i, lineFeedSize );
        } else if( strlen( strrchr( tempBuf, '\n' ) ) + strlen( msg + i ) > 80  ) {
            // Get the position of the last space in the line
            char* returnPos = strrchr( tempBuf, ' ' );
            // Replace the space with a newline
            returnPos[0] = '\n';

            // Temp buffer to store the text after the space
            char wrapText[255];
            // Copy the text that was after the space into the buffer
            strcpy( wrapText, returnPos + 1 );

            // Write indent spacing for this line, and append the
            // wrapped text from after the space
            sprintf( returnPos + 1, "%s%s", indentedLineSpacing, wrapText );

            // Copy from the message
            strncpy( returnPos + strlen( returnPos ), msg + i, strlen( msg + i ) );
        } else {
            // Copy the last part of the messaage
            strncpy( tempBuf + strlen( tempBuf ), msg + i, strlen( msg + i ) );
        }
    }

    // Copy the new message back into the message variable
    strncpy( msg, tempBuf, msgSize );

    // Ensure there is a newline at the end of the new message
    if( msg[strlen( msg ) - 1] != '\n' ) {
        sprintf( msg + strlen( msg ), "\n" );
    }

    // Free the line spacing var
    free( indentedLineSpacing );
}

// Put all public functions into their own "namespace"
simplog_namespace const simplog = { writeLog, writeStackTrace, setLogDebugLevel, setLogFile, setLogSilentMode, setLineWrap, flushLog, loadConfig };
