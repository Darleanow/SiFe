#ifndef LOGGER_H
#define LOGGER_H

void logger_init(void);

void write_log(const char *text);

const char *get_log_buffer(void);

int is_log_updated(void);

void reset_log_updated(void);

#endif
