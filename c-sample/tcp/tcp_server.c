#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>

const int DEFAULT_PORT = 12345;

int getSocketPort(int sock);
void error(char *msg, int error_no) {
    printf("Error: %d,%s\n", error_no, msg);
    exit(1);    /* 1 is passed to parent process */
}

/*
    Accept a port argument. If none, use DEFAULT_PORT.
*/

int main(int argc, char **argv) {
    int sock_p; /* parent socket (file descriptor) */
    int sock_c; /* child socket */
    struct sockaddr_in addr; /* server address */
    int server_port;
    struct sockaddr_in addr_client;
    socklen_t len;
    int BACKLOG = 5; /* Refuse connection if <BACKLOG> connection requests are waiting for accept */
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

    printf("get server port...\n");
    if ((server_port = getSocketPort(sock_p)) < 0) {
        close(sock_p);
        error("Error", errno);
    }
    printf("server_port=%d\n", server_port);

    len = sizeof(addr_client);
    printf("Waiting for connection...\n");
    sock_c = accept(sock_p, (struct sockaddr *)&addr_client, &len);
    if (sock_c < 0) {
        close(sock_p);
        error("Error", errno);
    }
    printf("Received connection from %s...\n", inet_ntoa(addr_client.sin_addr));

    printf("Send 'HELLO\\n'...\n");
    int n = write(sock_c, "HELLO\n", 6);
    if (n < 0) {
        close(sock_p);
        close(sock_c);
        error("Error", errno);
    }

    printf("Close client connection...\n");
    close(sock_c);

    printf("Stop listening...\n");
    close(sock_p);

    return 0;
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
