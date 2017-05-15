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
#define MAX_IP_COUNT 20

unsigned int g_child_id = 0; //Globally managed child id
int ipcompare(struct sockaddr_in *ip, struct sockaddr_in *ip_other);

typedef struct {
    /* shared data */
    volatile int is_alive;
    unsigned int child_id;

    /* child data (write-once-only by parent, then read-only by child) */
    int sock;
    char msg1[4096];
    char msg2[4096];
    char msg3[4096];
} T_CHILD_THREAD_DATA;

typedef struct {
    struct sockaddr_in ip;
    int conn_max;
    pthread_mutex_t mutex; //protect child_arr
    T_CHILD_THREAD_DATA *child_arr; //array of size conn_max
} T_IP_CHILDREN;

T_IP_CHILDREN ip_children[MAX_IP_COUNT];
int ip_children_length = 0;

/*
 * Arguments:
 *                 ip : allowed ip
 *           conn_max : allowed maximum connections for this ip
 *
 * Return   :
 *             n >= 0 : Success. n is the total count of allowed ip
 *                 -1 : Invalid ip. Config repeated.
 *                 -2 : Invalid conn_max. conn_max MUST >0
 */
int add_ip_children_config(struct sockaddr_in ip, int conn_max) {
    if (ip_children_length >= MAX_IP_COUNT) return -1; //No rooms for new IP!!
    
    /* parse ip
    struct sockaddr_in ip;
    int temp = inet_pton(AF_INET, ip_str, &ip);
    if (temp == 0) {
        //Invalid IP address
        return -1;
    } else {
        //Error, see errno
        return -1;
    }
    */
    
    for (int i=0;i<ip_children_length; i++) {
        if (ipcompare(&(ip_children[i].ip), &ip))
            return -1; // config conflict
    }
    
    if (conn_max < 0) return -2;

    // All checking okay, new config entry!
    T_IP_CHILDREN *entry = &(ip_children[ip_children_length]);
    entry->conn_max = conn_max;
    pthread_mutex_init(&(entry->mutex), NULL);
    entry->child_arr = malloc(sizeof(T_CHILD_THREAD_DATA) * conn_max);
    for (int i=0; i<conn_max; i++) {
        T_CHILD_THREAD_DATA *data = &(entry->child_arr[i]);
        data->is_alive = 0;
    }
    ip_children_length++;
    return ip_children_length;
}

/*
 * Arguments:
 *            p : Address of pointer. That pointer will stores the data. WARNING: Won't check *p==NULL, please be careful by yourself!!!!
 *           ip : incoming ip
 *
 * Return   :
 *            1 : Success, you can read *p
 *           -1 : Invalid ip, the incoming ip is NOT authorized in config.
 *           -2 : Connections full. Too many alive connections for this ip. The connection maximum (being set in config) has been attained.
 */
int new_child(T_CHILD_THREAD_DATA **p, struct sockaddr_in ip) {
    T_IP_CHILDREN *entry = NULL;
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *e = &(ip_children[i]);
        if (ipcompare(&(e->ip), &ip)) {
            entry = e;
            break;
        }
    }
    
    if (entry == NULL) return -1; // Invalid ip.
    
    int conn_max = entry->conn_max;
    T_CHILD_THREAD_DATA *data = NULL;
    
    // Find non-alive child
    for (int i=0; i<conn_max; i++) {
        T_CHILD_THREAD_DATA *d = &(entry->child_arr[i]);
        if (!(d->is_alive))
            data = d;
    }
    
    if (data == NULL) return -2; // All alive child, already conn_max!!
    
    // Write new child, need to lock (write, other might be reading)
    pthread_mutex_lock(&(entry->mutex));
    data->is_alive = 1;
    data->child_id = g_child_id;
    pthread_mutex_unlock(&(entry->mutex));
    
    g_child_id++;
    
    *p = data;
    
    return 1;
}

/*
 * Arguments:
 *            child_id : child id assigned
 *
 * Return   :
 *            non-NULL : Success.
 *            NULL     : child_id not found
 */
struct sockaddr_in *get_child_ip(unsigned int child_id) {
    struct sockaddr_in *ret = NULL;
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *entry = &(ip_children[i]);
        pthread_mutex_lock(&(entry->mutex));
        for (int j=0; j<entry->conn_max; j++) {
            T_CHILD_THREAD_DATA *data = &(entry->child_arr[j]);
            if (child_id == data->child_id) {
                ret = &(entry->ip);
                break;
            }
        }
        pthread_mutex_unlock(&(entry->mutex));
        if (ret != NULL) break;
    }
    
    return ret;
}

int close_child(T_CHILD_THREAD_DATA *data) {
    data->is_alive = 0;
    return 1;
}



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

    // Read config
    // init_child_data_arr();

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
        pthread_t child_thread;
        T_CHILD_THREAD_DATA *data=NULL;
        temp = new_child(&data, addr_client);
        if (temp < 0) {
            if (temp==-1) printf("Too many connections\n");
            close(sock_c);
            continue;
        }
        data->sock = sock_c;
        strcpy(data->msg1, "HELLO! This is message 1.\n");
        strcpy(data->msg2, "HELLO! This is message 2.\n");
        strcpy(data->msg3, "HELLO! This is message 3.\n");
        
        printf("Create child thread...\n");
        temp = pthread_create(&(child_thread), NULL, connectionHandler, (void *)data);
        if (temp != 0) {
            close(sock_c);
            error("Error", temp);
            break;
        }
        
        printf("Detach child thread....\n");
        temp = pthread_detach(child_thread);
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

void *connectionHandler(void *input_ptr) {
    long tid = get_tid();
    T_CHILD_THREAD_DATA* child_data = (T_CHILD_THREAD_DATA *)input_ptr;

    char read_msg[STR_BUF_MAX];
    char send_msg[STR_BUF_MAX];
    
    int child_id;
    int sock;
    struct sockaddr_in *addr_client_ptr;
    char msg1[STR_BUF_MAX];
    char msg2[STR_BUF_MAX];
    char msg3[STR_BUF_MAX];
    
    printf("%ld:Enter connection handler...\n", tid);
    
    printf("%ld:Reading child_data...\n", tid);
    child_id = child_data->child_id;
    sock = child_data->sock;
    addr_client_ptr = get_child_ip(child_id);
    strcpy(msg1, child_data->msg1);
    strcpy(msg2, child_data->msg2);
    strcpy(msg3, child_data->msg3);
    printf("%ld:child_id=%d, is_alive=%d, ip=%s, sock=%d...\n", tid,
        child_data->child_id,
        child_data->is_alive,
        inet_ntoa(addr_client_ptr->sin_addr),
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

    close_child(child_data);
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

/*
 * Return 1 if same ip and use AF_INET, otherwise return 0.
 */
int ipcompare(struct sockaddr_in *ip, struct sockaddr_in *ip_other) {
    if ((ip->sin_addr.s_addr == ip_other->sin_addr.s_addr)
     && (ip->sin_family == ip_other->sin_family)
     && (ip->sin_family == AF_INET)) {
        return 1;
    }
    return 0;
}
