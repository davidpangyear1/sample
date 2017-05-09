#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define STRING_BUFFER_MAX 4096 /* include the '\0' char  */

int interact(int sock, char *cmd);
int send_to(int sock, char *msg);
int read_line(int sock, char *line_buf, int buflen);

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
    temp = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if (temp < 0){
        printf("Error:%d\n", errno);
        return 0;
    }
    
    if (interact(sock, "msg1\n") < 0) {
        close(sock);
        return 0;
    }
    
    if (interact(sock, "msg2\n") < 0) {
        close(sock);
        return 0;
    }
    
    if (interact(sock, "msg3\n") < 0) {
        close(sock);
        return 0;
    }
    
    if (interact(sock, "msg4\n") < 0) {
        close(sock);
        return 0;
    }
    
    while (1) {
        if (interact(sock, "exit\n") < 0) {
            break;
        }
        sleep(5);
    }

    printf("Close connection...\n");
    close(sock);

    return 0;
}

/*
 * return:
 *     n(>0):strlen(read_msg)
 *     0:EOF, no LF detected.
 *     -1:Error (with errno)
 *     -2:Line too long, discard.
 */
int interact(int sock, char *cmd) {
    int temp;
    //char send_msg[STRING_BUFFER_MAX];
    char read_msg[STRING_BUFFER_MAX];
    
    printf("Sending command %s...\n",  cmd);
    temp = send_to(sock, cmd);
    if (temp < 0) {
        printf("Error:%d\n", errno);
        return 0;
    }

    printf("Receive message line...\n");
    temp = read_line(sock, read_msg, STRING_BUFFER_MAX);
    if (temp > 0) printf(read_msg);
    else interact_error(temp);
    
    return 0;
}

void interact_error(int temp) {
    if (temp == -2) printf("\nLine too long, discard.(max=%d)\n", STRING_BUFFER_MAX);
    else if (temp == -1) printf("\nError: %d\n", errno);
    else if (temp == 0) printf("\nEOF, no LF detected.\n");
}

int send_to(int sock, char *msg) {
    int temp;
    temp = write(sock, msg, (strlen(msg) + 1) * sizeof(char));
    if (temp < 0) {
        close(sock);
        return -1;
    }
    
    return 0;
}

/*
 * return:
 *     strlen(line_buf),
 *     0(No LF read, although EOF already),
 *     -1(Error with errno),
 *     -2(Line too long)
 */
int read_line(int sock, char *line_buf, int buflen) {
    char c;
    char buf[1];
    int n;
    int i = 0;
    while (1) {
        n = read(sock, buf, 1);
        if (n <= 0) return n; // -1 or 0
        c = buf[0];

        if (c == '\0') continue;
        if (c == '\r') continue;
        
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || (c == ' ')
            || (c == '\n')){
                //Valid character
            }
        else continue;
        
        line_buf[i] = c;
        i++;
        //trace: printf("%d\n", (int)buf[0]);
        if (c == '\n') {
            line_buf[i] = '\0';
            return i;
        }

        /*
         * Reserve two characters for "\n\0"
         */
        if (!(i < buflen - 1)) return -2;
    }
}
