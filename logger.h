/**
* Logger that runs in its own thread.
*/

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

#define MAX_MESSAGE_SIZE 256


enum log_level {
	TRACE = 1,
	DEBUG = 2,
	INFO = 3,
	WARN = 4,
	ERROR = 5
};


// Prints log message with specified level to stdout. 
extern int log_message(enum log_level lvl, char* mesasge);


// sets the global log level.  Will print anything above set log level.
extern int set_log_level(enum log_level lvl);

// creates mutex lock.
extern int setup_logger();

// frees mutex
extern int destroy_logger();



#endif