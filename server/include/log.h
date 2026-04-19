#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

typedef enum {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL
} log_level_t;

int log_init(const char *filename);
void log_set_level(log_level_t level);
void log_message(log_level_t level, const char *format,...);
void log_access(const char *client_ip, const char *method, const char *uri,int status,long response_size);
void log_cleanup(void);

#define LOG_DEBUG(...) log_message(LOG_DEBUG,__VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __VA_ARGS__)

#endif
