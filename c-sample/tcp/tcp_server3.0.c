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
#define MAX_CONNECTION 20

typedef struct {
    pthread_mutex_t mutex;
    int ind;
    int is_using;

    pthread_t t;

    struct sockaddr_in addr_client;
    int sock;

    char msg1[4096];
    char msg2[4096];
    char msg3[4096];
} T_CHILD_THREAD_DATA;

T_CHILD_THREAD_DATA child_data_arr[MAX_CONNECTION];

void init_child_data_arr();
int new_child_data(T_CHILD_THREAD_DATA **ptr);
void release_child_data(T_CHILD_THREAD_DATA *p);




void *connectionHandler(void *arg_ptr);
int read_line(int sock, char *line_buf, int buflen);
int getSocketAddr(int sock, char* buf, int buflen);
int getSocketPort(int sock);
long get_tid();
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
    
    int temp;

    init_child_data_arr();

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

    while (1) {
        printf("Waiting for connection...\n");
        len = sizeof(addr_client);
        int sock_c = accept(sock_p, (struct sockaddr *)&addr_client, &len);
        if (sock_c < 0) {
           error("Error", errno);
           break;
        }
        printf("Received connection from %s...\n", inet_ntoa(addr_client.sin_addr));

        printf("Create child data...\n");
        T_CHILD_THREAD_DATA *data=NULL;
        temp = new_child_data(&data);
        if (temp < 0) {
            if (temp==-1) printf("Too many connections\n");
            close(sock_c);
            continue;
        }
        data->addr_client = addr_client;
        data->sock = sock_c;
        strcpy(data->msg1, "HELLO! This is message 1.\n");
        strcpy(data->msg2, "HELLO! This is message 2.\n");
        strcpy(data->msg3, "HELLO! This is message 3.\n");
        
        printf("Create child thread...\n");
        temp = pthread_create(&(data->t), NULL, connectionHandler, (void *)data);
        if (temp != 0) {
            close(sock_c);
            error("Error", temp);
            break;
        }
        
        printf("Detach child thread....\n");
        temp = pthread_detach(data->t);
        if (temp != 0) {
            close(sock_c);
            error("Error", temp);
            break;
        }
    }

    printf("Stop listening...\n");
    close(sock_p);

    return 0;
}

void init_child_data_arr() {
    for (int i=0; i<MAX_CONNECTION; i++) {
        T_CHILD_THREAD_DATA *p = &(child_data_arr[i]);
        pthread_mutex_init(&(p->mutex), NULL);
        p->ind = i;
        p->is_using = 0;
    }
}

// Don't check null pointer. Please be careful yourself.
// -1: "too many connections";
int new_child_data(T_CHILD_THREAD_DATA **ptr) {
    T_CHILD_THREAD_DATA *ret;
    int conn_max;
    int ind = -1;
    int conn_count = 0;
    
    conn_max = MAX_CLIENT;
    
    //Count current connections
    for (int i=0; i<MAX_CONNECTION; i++) pthread_mutex_lock(&(child_data_arr[i].mutex));
    for (int i=0; i<MAX_CONNECTION; i++)  {
        T_CHILD_THREAD_DATA *data = &(child_data_arr[i]);
        if (data->is_using) {
            conn_count++;
        } else {
            // Just remember ind here, to reduce the locking time.
            if (ind == -1) ind = i; // The checking is not necessary.
        }
    }
    for (int i=0; i<MAX_CONNECTION; i++) pthread_mutex_unlock(&(child_data_arr[i].mutex));
    
    if (conn_count > conn_max) return -1;
    ret = &(child_data_arr[ind]);
    ret->is_using = 1;
    
    *ptr = ret;
    
    return 0;
}

void release_child_data(T_CHILD_THREAD_DATA *p) {
    p->is_using = 0;
}

void *connectionHandler(void *input_ptr) {
    long tid = get_tid();
    T_CHILD_THREAD_DATA* child_data = (T_CHILD_THREAD_DATA *)input_ptr;

    char read_msg[STR_BUF_MAX];
    char send_msg[STR_BUF_MAX];
    
    int sock;
    char msg1[STR_BUF_MAX];
    char msg2[STR_BUF_MAX];
    char msg3[STR_BUF_MAX];
    
    printf("%ld:Enter connection handler...\n", tid);
    
    printf("%ld:Reading child_data...\n", tid);
    sock = child_data->sock;
    strcpy(msg1, child_data->msg1);
    strcpy(msg2, child_data->msg2);
    strcpy(msg3, child_data->msg3);
    printf("%ld:child=%d, is_using=%d, &t=%p, ip=%s, %d...\n", tid,
        child_data->ind,
        child_data->is_using,
        &(child_data->t),
        inet_ntoa(child_data->addr_client.sin_addr),
        child_data->sock
    );
    
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

    release_child_data(child_data);
    return NULL;
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
