#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* The length of the log message. */
# define MAX_LOG_LEN 4096

const char DEFAULT_LOG_PATH[] = "./output.log";
int logline (char *fmt, ...);

static FILE *logfp = NULL;
static int pid = 0;
char *get_current_date_time();
int logline(char *fmt, ...);

int logline(char *fmt, ...) {
    char msg[MAX_LOG_LEN];
    va_list tArgptr;

    /* Better use logging, update later */

    //pthread_mutex_lock(mutex);

    va_start(tArgptr, fmt);
    vsprintf(msg, fmt, tArgptr);
    va_end(tArgptr);

    //write log file
    fprintf(logfp, "%s %5d  %s\n", get_current_date_time(), pid, msg);
    fflush(logfp);

    //pthread_mutex_unlock(mutex);

    return 1;
    
}

int main() {
    /* Get process id for logging */
    pid = getpid(); 
    
    /* open logging file */
    if (logfp == NULL) {
        logfp = fopen(DEFAULT_LOG_PATH, "a");
        if (logfp == NULL) return -1;
    }
    
    if (logline("==============================Start Logging==============================") < 0) {
        printf("Logging failed:%d\n", errno);
        return 0;
    }

    for (int i = 0; i < 10; i++) {
        if (logline("Hello Again, World!! %d", i) < 0) 
            printf("Logging failed:%d\n", errno);
    }

    /* close logging file */
    if (logfp != NULL) fclose(logfp);

    return 0;
}

char *get_current_date_time(void) {
    char temp[100];
    time_t rawtime;
    struct tm *datetime;

    //struct timezone zone;
    struct timeval tv;
    int msec;
    
    static char ret[100]; //  static: must not return local variable

    time(&rawtime);
    datetime = localtime(&rawtime);
    strftime(temp, sizeof(temp), "%Y-%m-%d %H:%M:%S", datetime);

    gettimeofday(&tv, NULL);    //gettimeofday(&tv, &zone);
    msec = (int)(tv.tv_usec / 1000);
    sprintf(ret, "%s.%03d", temp, msec);
    
    /* Another way to obtain the time
    sprintf(ret, "%4d-%02d-%02d,%02d:%02d:%02d.%06d,", 
        datetime->tm_year + 1900,
        datetime->tm_mon + 1,
        datetime->tm_mday,
        datetime->tm_hour,
        datetime->tm_min,
        datetime->tm_sec,
        tv.tv_usec
    );
    */

    return ret;
}
