#SimpleLogger

A basic, easy to use logger for output of messages at various logging levels
with date/time stamp to standard out and a defined log file.

![Example Screenshot](http://i.imgur.com/1aMpk6I.png)

##Features

* Time/date stamped output
* Console output is colored according to the level of the log message
* Error levels automatically print errno if it has been set
* Variable length argument list
* Supports writing of stacktraces.  Stacktraces are "prettyfied" to give the
user an easiear to read trace
* Has an option to wrap messages over 80 characters to multiple lines
* Written in C, for use in C and C++ projects

##Usage

Each message must be accompanied by a level. Supported levels are:
```
SIMPLOG_FATAL   = -2 : A fatal error has occured, the program will exit immediately
SIMPLOG_ERROR   = -1 : An error has occured, the program may not exit
SIMPLOG_INFO    = 0  : Nessessary information regarding program operation
SIMPLOG_WARN    = 1  : Any circumstance that may not affect normal operation
SIMPLOG_DEBUG   = 2  : Standard debug messages
SIMPLOG_VERBOSE = 3  : All debug messages
```

Here is an example of how to create a log entry:
```
simplog.writeLog( <desired_level>, "Message", <var-args> );
```

For example, to recreate part of the output in the screenshot above:
```
simplog.writeLog( SIMPLOG_FATAL, "Test Fatal" );
simplog.writeLog( SIMPLOG_ERROR, "Test Error" );
simplog.writeLog( SIMPLOG_INFO, "Test Info" );
simplog.writeLog( SIMPLOG_WARN, "Test Warn" );
simplog.writeLog( SIMPLOG_DEBUG, "Test Debug" );
simplog.writeLog( SIMPLOG_VERBOSE, "Test Verbose" );
```

To write a stacktrace, just call:
```
simplog.writeStackTrace();
```

If you want to load a config file to setup the logger:
```
simplog.loadConfig( <desired-config-file> );
```
With a config that can use any of the following options:
```
silent=<true-or-false>    // Sets the silent mode
wrap=<true-or-false>      // Sets if too long lines are wrapped
flush=<true-or-false>     // If the log file should be flushed or not
debug=<0-to-3>            // Set the debug level, 0 - 3
logfile=<logfile-path>    // Set the path to the logfile to output to
```
There are individual functions to set each of these parameters manually as well.  See the documentation below.

##Documentation

###writeLog
```
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
void writeLog( int loglvl, const char* str, ... );
```

###writeStackTrace
```
/*
    Prints the stacktrace to logs for the current location in the program.
    Most recent calls appear first.

    Set to a max of 15 lines of the stacktrace for output.
*/
void writeStackTrace();
```

###loadConfig
```
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

void loadConfig( const char* config );
```
###setLogDebugLevel
```
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
void setLogDebugLevel( int level );
```

###setLogFile
```
/*
    Sets the filename for log output.

    Input:
    const char* file - desired log output file
*/
void setLogFile( const char* file );
```

###setLogSilentMode
```
/*
    Enables/Disables silent mode.
    When silent mode is enabled, no output will be written to standard out.
    Log output will continue normally.

    Input:
    bool silent - Desired state of silent mode: false = Disabled (default), true = Enabled
*/
void setLogSilentMode( bool silent );
```

###setLineWrap
```
/*
    Enables/Disables line wrapping.
    When line wrapping is enabled, lines that are over 80 characters will be 
    wrapped multiple times so that each line is below 80 characters.

    Input:
    bool wrap - Desired state of line wrapping: true = Enabled (default), false = Disabled
*/
void setLineWrap( bool wrap );
```

###flushLog
```
/*
    Flushes the contents of the logfile by deleting it and recreating
    a new empty logfile of the same name.
*/
void flushLog();
```
