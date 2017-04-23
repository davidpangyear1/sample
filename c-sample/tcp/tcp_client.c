#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int main() {
    int sock;
    struct sockaddr_in server;
    int PORT = 12345;

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

    printf("Receive message...\n");
    char buf[32];
    int n = read(sock, buf, sizeof(buf));
    if (n < 0) { printf("Error:%d\n", errno); return 0; }
    if (n == 0) { printf("EOF\n"); return 0; }
    buf[n] = '\0';
    printf("%s\n",buf);

    printf("Close connection...\n");
    close(sock);

    return 0;
}
