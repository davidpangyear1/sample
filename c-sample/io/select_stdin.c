#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define STDIN 0 // file descriptor of standard input

int main(void) {
    struct timeval timeout;
    fd_set readfds; /* set of fd to be read */
    int temp;

while (1) {
    printf("Configuring...\n");
    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds); /* add STDIN into readfds */

    /* The timeout value might be changed by select.
     * For safety, please reset it EVERY time.
     */
    timeout.tv_sec = 12;
    timeout.tv_usec = 500000; /* in microsecond */

    /*
     * note that terminal will buffer your input until you press enter.
     */
    printf("Press enter after you have typed your things. Timeout in: %ld ms.\n", timeout.tv_sec * 1000 + timeout.tv_usec / 1000);

    /*
     * select will block and monitor the fds.
     * If there are fd available,
     * those fd will be put into corresponding fd_set,
     * and then the select method will return the total count (of all fd).
     *
     * If the timeout is non-NULL, the return value might be 0.
     *
     * Return -1 if error. In this case, view the errorno. 
     */
    temp = select(STDIN + 1, &readfds, NULL, NULL, &timeout);
    if (temp < 0) {
        printf("Error,%d\n", errno);
        return 0;
    }

    if (temp == 0) {
        printf("Timed out.\n");
        break;
    }

    /* Check if STDIN is in readfds  */
    if (FD_ISSET(STDIN, &readfds)) {
        printf("Received(Except square bracket):\n[");
        char buf[1024];
        int len;
        while (1) {
            len = read(STDIN, &buf, 1024);
            /* nothing to print */
            if (len <= 0) break;

            printf("%s", buf);

            /* nothing left */
            if (len < 1024) break;

            memset(buf, 0, sizeof(buf));
        }
        printf("]\nEnd read\n");
    }
}
    return 0;
}
