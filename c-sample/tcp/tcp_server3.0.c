#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <regex.h>

#define STR_BUF_MAX 4096
#define DEFAULT_PORT 12345
#define BACKLOG 5 /* Refuse connection if <BACKLOG> connection requests are waiting for accept */
#define MAX_CLIENT 10

typedef struct {
    int sock;
    char msg1[4096];
    char msg2[4096];
    char msg3[4096];
} thread_context;

void *connectionHandler(void *arg_ptr);
int getSocketAddr(int sock, char* buf, int buflen);
int getSocketPort(int sock);
long get_tid();
int read_line(int sock, char *line_buf, int buflen);
void error(char *msg, int error_no) {
    printf("Error: %d,%s\n", error_no, msg);
    exit(1);    /* 1 is passed to parent process */
}

/*
 *   Accept a port argument. If none, use DEFAULT_PORT.
 *   Will listen for clients and send them Hello.
 */

int main(int argc, char **argv) {
    int sock_p; /* parent socket (file descriptor) */
    struct sockaddr_in addr; /* server address */
    int server_port;
    char server_addr[STR_BUF_MAX];
    struct sockaddr_in addr_client;
    int len; //socklen_t len;

    pthread_t t[MAX_CLIENT];

    int temp;

    /* check command line arguments */
    if (argc < 2) {
        server_port = DEFAULT_PORT;
        printf("usage: %s <port>\nNow use default port=%d\n", argv[0], server_port);
    } else {
        server_port = atoi(argv[1]);
    }

    printf("Create socket...\n");
    sock_p = socket(AF_INET, SOCK_STREAM, 0); /* AF_INET means IPv4 */
    if (sock_p < 0) error("ERROR opening socket", errno);

    printf("Configure socket...\n");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port); /* htons make sure the bytes stored in memory are ordered in network byte order  */
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Set socket option in order to reuse the port. Otherwise, restart this app will cause 'bind' fail. */
    printf("Set socket option...\n");
    int sock_opt = -1;
    temp = setsockopt(sock_p, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt, sizeof(sock_opt));
    if (temp  < 0) error("Error", errno);

    printf("bind...\n");
    temp = bind(sock_p, (struct sockaddr*)&addr, sizeof(addr));
    if (temp < 0) {
        close(sock_p);
        error("Error", temp);
    }

    printf("listen...\n");
    temp = listen(sock_p, BACKLOG);
    if (temp < 0 ) {
        close(sock_p);
        error("Error", temp);
    }

    printf("get server address...\n");
    temp = getSocketAddr(sock_p, server_addr, STR_BUF_MAX);
    if (temp < 0) {
        close(sock_p);
        error("Error", temp);
    }
    printf("server_addr=%s\n", server_addr);

    int c_ind = 0;
    while (c_ind < MAX_CLIENT) {
        printf("Waiting for connection...\n");
        len = sizeof(addr_client);
        int sock_c = accept(sock_p, (struct sockaddr *)&addr_client, &len);
        if (sock_c < 0) {
           close(sock_p);
           error("Error", errno);
        }
        printf("Received connection from %s...\n", inet_ntoa(addr_client.sin_addr));

        printf("Create thread context...\n");
        thread_context *tc_ptr = malloc(sizeof(thread_context));
        tc_ptr->sock = sock_c;
        strcpy(tc_ptr->msg1, "HELLO! This is message 1.\n");
        strcpy(tc_ptr->msg2, "HELLO! This is message 2.\n");
        strcpy(tc_ptr->msg3, "HELLO! This is message 3.\n");
        
        printf("Create child thread...\n");
        temp = pthread_create(&t[c_ind], NULL, connectionHandler, (void *)tc_ptr);
        if (temp != 0) {
            close(sock_c);
            close(sock_p);
            error("Error", temp);
        }
        c_ind++;
    }

    printf("Waiting until all connections close...\n");
    for (int i = 0; i < MAX_CLIENT; i++) {
        temp = pthread_join(t[i], NULL);
        if (temp != 0) {
            close(sock_p);
            error("Error", temp);
        }
    }

    printf("Stop listening...\n");
    close(sock_p);

    return 0;
}

void *connectionHandler(void *input_ptr) {
    long tid = get_tid();
    thread_context* tc_ptr = (thread_context *)input_ptr;

    char read_msg[STR_BUF_MAX];
    char send_msg[STR_BUF_MAX];
    
    int sock;
    char msg1[STR_BUF_MAX];
    char msg2[STR_BUF_MAX];
    char msg3[STR_BUF_MAX];
    
    printf("%ld:Enter connection handler...\n", tid);
    
    printf("%ld:Reading thread context...\n", tid);
    sock = tc_ptr->sock;
    strcpy(msg1, tc_ptr->msg1);
    strcpy(msg2, tc_ptr->msg2);
    strcpy(msg3, tc_ptr->msg3);
    free(tc_ptr);

    int flag_cont = 1;
    while (flag_cont) {
        /* Read query from client */
        printf("%ld:Reading message from %d...\n", tid, sock);
        int n = read_line(sock, read_msg, STR_BUF_MAX);
        if (n > 0) {
            /* Process input */
            printf("%ld:Get message, n=%d, %s...\n", tid, n, read_msg);
            if (strcmp(read_msg, "exit\n") == 0) {
                strcpy(send_msg, "exit received\n");
                flag_cont = 0;
            } else if (strcmp(read_msg, "heartbeat\n") == 0) {
                strcpy(send_msg, "heartbeat received\n");
            } else if (strcmp(read_msg, "msg1\n") == 0) {
                strcpy(send_msg, msg1);
            } else if (strcmp(read_msg, "msg2\n") == 0) {
                strcpy(send_msg, msg2);
            } else if (strcmp(read_msg, "msg3\n") == 0) {
                strcpy(send_msg, msg3);
            } else {
                strcpy(send_msg, "Unknown Command\n");
            }

            /* Send message */
            printf("%ld:Sending message, %s...\n", tid, send_msg);
            int n = write(sock, send_msg, (strlen(send_msg) + 1) * sizeof(char));
            if (n < 0) {
                close(sock);
                fprintf(stderr, "%ld:Error on connection handler, %d\n", tid, errno);
                return (void *)(&errno);
            }
        } else if (n < 0) {
            close(sock);
            fprintf(stderr, "%ld:Error on connection handler read_line, %d, %d\n", tid, n, errno);
            return (void *)(&errno);
        } else if (n == 0) {
            printf("%ld:Nothing to read...\n", tid);
            break;
        }
    }
    printf("%ld:Close client connection...\n", tid);
    close(sock);

    return NULL;
}

int getSocketAddr(int sock, char* buf, int buflen) {
    if (!buf || buflen < 1) 
        return -1; // bad input.

    struct sockaddr_in sin;
    int addrlen = sizeof(sin);
    char str[4096];
    char **ptr = NULL;
    char *ip;
    int port;
    if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
            && sin.sin_family == AF_INET
            && addrlen == sizeof(sin)) {
        ip = inet_ntoa(sin.sin_addr);
        port = ntohs(sin.sin_port);
        int len = sprintf(str, "%s:%d", ip, port);

        if (len >= buflen - 1) return -2; //Not enough buffer

        strcpy(buf, str);
        return strlen(buf);
    } else {
        buf = '\0';
        return errno;
    }
}

int getSocketPort(int sock) {
    struct sockaddr_in sin;
    int addrlen = sizeof(sin);

    if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
            && sin.sin_family == AF_INET
            && addrlen == sizeof(sin)) {
        return ntohs(sin.sin_port);
    } else {
        return -1;
    }
}

/*
 * get the pointer of current thread as thread id
 */
long get_tid() {
    pthread_t self_t;
    self_t = pthread_self();
    long ret = (long)&self_t;
    return ret;
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
