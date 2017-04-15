#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

const int DEFAULT_PORT = 12345;
const int BACKLOG = 5; /* Refuse connection if <BACKLOG> connection requests are waiting for accept */
const int MAX_CLIENT = 10;

void *connectionHandler(void *arg_ptr);
char *getSocketAddr(int sock);
int getSocketPort(int sock);
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
    char *server_addr;
    struct sockaddr_in addr_client;
    socklen_t len;

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
    if (temp  < 0) {
        close(sock_p);
        error("Error", errno);
    }

    printf("listen...\n");
    if (listen(sock_p, BACKLOG) < 0 ) {
        close(sock_p);
        error("Error", errno);
    }

    printf("get server address...\n");
    if ((server_addr = getSocketAddr(sock_p)) == '\0') {
        close(sock_p);
        error("Error", errno);
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

        printf("Create child thread...\n");
        temp = pthread_create(&t[c_ind], NULL, connectionHandler, (void *)&sock_c);
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

void *connectionHandler(void *arg_ptr) {
    long tid = get_tid();
    printf("%ld:Enter connection handler...\n", tid);
    int sock_c = *((int *)arg_ptr);

    /* Send message */
    char *msg = "HELLO\n";
    printf("%ld:Send %s...\n", tid, msg);
    int n = write(sock_c, msg, 6);
    if (n < 0) {
        close(sock_c);
        fprintf(stderr, "%ld:Error on connection handler, %d\n", tid, errno);
        return (void *)(&errno);
    }

    printf("%ld:Close client connection...\n", tid);
    close(sock_c);
}

char *getSocketAddr(int sock) {
    struct sockaddr_in sin;
    int addrlen = sizeof(sin);
    char *ret;
    char *ip;
    int port;
    if (getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0
            && sin.sin_family == AF_INET
            && addrlen == sizeof(sin)) {
        ip = inet_ntoa(sin.sin_addr);
        port = ntohs(sin.sin_port);
        asprintf(&ret, "%s:%d", ip, port); /* from GNU extension */
        return ret;
    } else {
        return '\0';
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
