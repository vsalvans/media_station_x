#ifndef LOGGER_H
#define LOGGER_H



enum log_level {
	ERROR,
	INFO,
	DEBUG
};

void init_logger(enum log_level);

void log_error(const char * fmt, ...);
void log_info(const char * fmt, ...);
void log_debug(const char * fmt, ...);

void destroy_logger(void);

#endif
