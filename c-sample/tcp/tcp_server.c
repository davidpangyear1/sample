#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int getSocketPort(int sock);

int main(int argc, char **argv) {
    int sock_p; /* parent socket (file descriptor) */
    int sock_c; /* child socket */
    struct sockaddr_in addr; /* server address */
    int server_port;
    struct sockaddr_in addr_client;
    socklen_t len;
    int BACKLOG = 5; /* Refuse connection if <BACKLOG> connection requests are waiting for accept */

    /* check command line arguments */
    if (argc != 2) {
        server_port = 12345;
        printf("usage: %s <port>\nNow use default port=%d\n", argv[0], server_port);
    }

    /* Create socket, AF_INET means IPv4 */
    sock_p = socket(AF_INET, SOCK_STREAM, 0);

    /* Configure socket */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port); /* htons make sure the bytes stored in memory are ordered in network byte order  */
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Set socket option in order to reuse the port. Otherwise, restart this app will cause 'bind' fail. */
    printf("Set socket option...\n");
    int sock_opt = -1;
    if (setsockopt(sock_p, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt, sizeof(sock_opt)) < 0){
        printf("Error, %d\n", errno);
        return 0;
    }

    printf("bind...\n");
    if (bind(sock_p, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Error, %d, closing...\n", errno);
        close(sock_p);
        return 0;
    }

    printf("listen...\n");
    if (listen(sock_p, BACKLOG) < 0 ) {
        printf("Error, %d\n", errno);
        return 0;
    }

    printf("get server port...\n");
    if ((server_port = getSocketPort(sock_p)) < 0) {
        printf("Error\n");
    }
    printf("server_port=%d\n", server_port);

    len = sizeof(addr_client);
    printf("Waiting for connection...\n");
    sock_c = accept(sock_p, (struct sockaddr *)&addr_client, &len);
    printf("Received connection from %s...\n", inet_ntoa(addr_client.sin_addr));

    printf("Send hello...\n");
    write(sock_c, "HELLO\n", 6);

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
