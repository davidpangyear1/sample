#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

const int BUFFER_MAX = 65536;
/* LINE_LENGTH_MAX must < BUFFER_MAX */
const int LINE_LENGTH_MAX = 4096; /* include the '\n' char  */

int print_line(int sock, int max);

int main() {
    int sock;
    struct sockaddr_in server;
    int PORT = 12345;
    int temp;

    /* create socket, AF_INET means IPv4 */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    /* configure socket */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT); /* htons make sure the bytes stored in memory are ordered in network byte order  */

    printf("Connect to %d...\n", PORT);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("Error:%d\n", errno);
        return 0;
    } 

    printf("Receive message line...\n");
    temp = print_line(sock, LINE_LENGTH_MAX);
    if (temp == -3) printf("\nInvalid max line length!!!(must<%d)\n", BUFFER_MAX);
    else if (temp == -2) printf("\nLine too long, discard.(max=%d)\n", LINE_LENGTH_MAX);
    else if (temp == -1) printf("\nError: %d\n", errno);
    else if (temp == 0) printf("\nEOF, no LF detected.\n");

    printf("Close connection...\n");
    close(sock);

    return 0;
}

/* max: max count of characters, including the '\n' */
/* return strlen, 0, -1(with errno), -2(means line too long), -3(means invalid max length) */
int print_line(int sock, int max) {
    if (max >= BUFFER_MAX) return -3;

    char print_buf[BUFFER_MAX];
    char buf[1];
    int n;
    int i = 0;
    while (1) {
        n = read(sock, buf, 1);
        if (n <= 0) return n;

        print_buf[i] = buf[0];
        i++;

        if (buf[0] == '\n') {
            print_buf[i] = '\0';
            printf("%s", print_buf);
            return i;
        }

        if (i >= max) return -2;
    }
}
