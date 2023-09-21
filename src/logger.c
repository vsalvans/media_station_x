#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "logger.h"

#define LOG_FILEPATH "/var/log/media/media.log"

static FILE * LOGGER_FILE = NULL;

enum log_level CURRENT_LOG_LEVEL = INFO;

const char pathSeparator = '/';

// Given a file path, create all constituent directories if missing
void create_file_path_dirs(char *file_path) {
  char *dir_path = (char *) malloc(strlen(file_path) + 1);
  char *next_sep = strchr(file_path, pathSeparator);
  while (next_sep != NULL) {
    int dir_path_len = next_sep - file_path;
    memcpy(dir_path, file_path, dir_path_len);
    dir_path[dir_path_len] = '\0';
    if (dir_path_len != 0) {
      mkdir(dir_path, S_IRWXU|S_IRWXG|S_IROTH);
    }
    next_sep = strchr(next_sep + 1, pathSeparator);
  }
  free(dir_path);
}

void init_logger(enum log_level level) {
	if (NULL == LOGGER_FILE) {
    create_file_path_dirs(LOG_FILEPATH);
		LOGGER_FILE = fopen(LOG_FILEPATH, "a");
		if (NULL == LOGGER_FILE) {
			perror("Logger file coudn't be open");
		}
	}

	if (0 != level) {
		CURRENT_LOG_LEVEL = level;
	}
}

char * get_current_date(void) {
	char * date = (char *)malloc(100);

	time_t t = time(NULL);
	struct tm * p = localtime(&t);

	strftime(date, 100, "%F %T", p);
	return date;
}


void log_message(enum log_level level, const char * message) {
	if (NULL == LOGGER_FILE) {
		perror("Logger is no initialized");
		return;
	}

	if (CURRENT_LOG_LEVEL < level) {
		perror("Level is no enough");
		return;
	}

	char * level_str;

	if (ERROR == level) {
		level_str = "ERROR";
	} else if (INFO == level) {
		level_str = "INFO";
	} else if (DEBUG == level) {
		level_str = "DEBUG";
	} else {
		perror("Invalid log level");
		return;
	}

	char log_line[400];
	char * date = get_current_date();
	sprintf(log_line, "[%s] %s: %s\n", date, level_str, message);
	fputs(log_line, LOGGER_FILE);
	fflush(LOGGER_FILE);
	printf("%s", log_line);
	free(date);
}

void log_error(const char * fmt, ...) {
	char message[300];

	va_list args;
	va_start(args, fmt);
	vsprintf(message, fmt, args);
	va_end(args);
	
	log_message(ERROR, message);
}

void log_info(const char * fmt, ...) {
	char message[300];

	va_list args;
	va_start(args, fmt);
	vsprintf(message, fmt, args);
	va_end(args);
	
	log_message(INFO, message);
}

void log_debug(const char * fmt, ...) {
	char message[300];

	va_list args;
	va_start(args, fmt);
	vsprintf(message, fmt, args);
	va_end(args);
	
	log_message(DEBUG, message);
}

void destroy_logger(void) {
	if (NULL == LOGGER_FILE) {
		return;
	}

	fclose(LOGGER_FILE);
}	
