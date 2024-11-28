#include "logger.h"
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <string.h>
    #define STRCAT_SAFE(dest, destsz, src) strlcat(dest, src, destsz)
#else
#define STRCAT_SAFE(dest, destsz, src) strcat_s(dest, destsz, src)
#endif

#define LOG_BUFFER_SIZE 64000



static char logbuf[LOG_BUFFER_SIZE];
static int logbuf_updated = 0;

void logger_init(void) {
    logbuf[0] = '\0';
    logbuf_updated = 0;
}

void write_log(const char *text) {
    if (logbuf[0]) {
        if (STRCAT_SAFE(logbuf, sizeof(logbuf), "\n") >= sizeof(logbuf)) {
            fprintf(stderr, "Failed to append newline to log\n");
            return;
        }
    }

    if (STRCAT_SAFE(logbuf, sizeof(logbuf), text) >= sizeof(logbuf)) {
        fprintf(stderr, "Failed to append text to log\n");
        return;
    }

    logbuf_updated = 1;
}

const char* get_log_buffer(void) {
    return logbuf;
}

int is_log_updated(void) {
    return logbuf_updated;
}

void reset_log_updated(void) {
    logbuf_updated = 0;
}