

#include "logger.h"

// global variables
static pthread_mutex_t lock;
static enum log_level LEVEL = DEBUG;



int setup_logger()
{
	int resp = pthread_mutex_init(&lock, NULL);
	if (resp != 0)
	{
		// error create mutex lock.
		return -1;
	}
	return 0;
}

int destroy_logger()
{
	int resp = pthread_mutex_destroy(&lock);
	if (resp != 0)
	{
		// error destroying mutex
		return -1;
	}

	return 0;
}


int log_message(enum log_level lvl, char* message)
{
	if (strlen(message) > MAX_MESSAGE_SIZE ) {
		// message too big
		return -1;
	}

	if (pthread_mutex_lock(&lock) != 0)
	{
		// error locking lock
		return -1;
	}

	

	time_t current_time;
	struct tm* time_info;
	char timeString[9];
	timeString[8] = '\0';

	time(&current_time);
	time_info = localtime(&current_time);
	strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

	if (lvl >= LEVEL)
	{
		switch (lvl)
		{
			case TRACE:
				(void)printf("[%s TRACE] %s\n", timeString, message);
				break;
			
			case DEBUG:
				(void)printf("[%s DEBUG] %s\n",timeString, message);
				break;

			case INFO:
				(void)printf("[%s INFO] %s\n",timeString, message);
				break;

			case WARN:
				(void)printf("[%s WARN] %s\n",timeString, message);
				break;

			case ERROR:
				(void)printf("[%s WARN] %s\n",timeString, message);
				break;

			default:
				return -1;
				break;

		}
	}

	if (pthread_mutex_unlock(&lock) != 0)
	{
		// error unlocking lock
		return -1;
	}

	return 0;
}


// sets the global log level.  Will print anything above set log level.
// This function is not thread safe. Only call during setup.
int set_log_level(enum log_level lvl)
{
	LEVEL = lvl;
	return 0;
}