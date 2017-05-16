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




/* REGION START: Util*/
int issameipv4(struct sockaddr_in *ip, struct sockaddr_in *ip_other);
int read_line(int sock, char *line_buf, int buflen);
int getSocketAddr(int sock, char* buf, int buflen);
int getSocketPort(int sock);
void error(char *msg, int error_no) {
    printf("Error: %d,%s\n", error_no, msg);
    exit(1);    /* 1 is passed to parent process */
}
/* REGION END */




/* REGION START: Connection control */
#define MAX_IP_COUNT 20
typedef struct {
    /* shared data */
    volatile int is_alive;
    unsigned int child_id; // 0 means NOT set
    pthread_t thr;

    /* child data (write-once-only by parent, then read-only by child) */
    int sock;
    char msg1[4096];
    char msg2[4096];
    char msg3[4096];
} T_CHILD_THREAD_DATA;
unsigned int child_id_g = 1; //Globally managed child id

typedef struct {
    struct sockaddr_in addr;
    int conn_max;
    pthread_mutex_t mutex; //protect child_arr
    T_CHILD_THREAD_DATA *child_arr; //array of size conn_max
} T_IP_CHILDREN;

T_IP_CHILDREN ip_children[MAX_IP_COUNT];
int ip_children_length = 0;

void print_ip_children();
int add_ip_children_config(char *ip_str, int conn_max);
int new_child(T_CHILD_THREAD_DATA **p, struct sockaddr_in addr);
struct sockaddr_in *get_child_addr(unsigned int child_id);
int cancel_all_child(struct sockaddr_in addr);

void print_ip_children() {
    printf("print_ip_children():\n");
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *entry = &(ip_children[i]);
        
        //ip
        struct sockaddr_in addr = entry->addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(addr.sin_family, &(addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
        
        //conn_max
        int conn_max = entry->conn_max;
        
        printf("    %s, %d\n", ip_str, conn_max);
        
        //child_arr
        pthread_mutex_lock(&(entry->mutex));
        for (int j=0; j<conn_max; j++) {
            T_CHILD_THREAD_DATA* data = &(entry->child_arr[j]);
            
            // is_alive
            int is_alive = data->is_alive;
            
            // child_id
            int child_id = data->child_id;
            
            printf("        is_alive=%d, child_id=%d\n", is_alive, child_id);
        }
        pthread_mutex_unlock(&(entry->mutex));
    }
}

/*
 * Arguments:
 *             ip_str : allowed ip, in readable string (IPv4)
 *           conn_max : allowed maximum connections for this ip
 *
 * Return   :
 *             n >= 0 : Success. n is the total count of allowed ip
 *                 -1 : Invalid arguments.
 *                 -2 : ip already configurated, or too much ip.
 *                 -3 : Parse error, see errno.
 */
int add_ip_children_config(char *ip_str, int conn_max) {
    if (ip_children_length >= MAX_IP_COUNT) return -2; //No rooms for new IP!!

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

    // parse ip
    int temp = inet_pton(addr.sin_family, ip_str, &(addr.sin_addr.s_addr));
    if (temp == 0) return -1; //Invalid IP address
    if (temp == -1) return -3; //Error, see errno
    
    // check if the ip is set already
    for (int i=0;i<ip_children_length; i++) {
        if (issameipv4(&(ip_children[i].addr), &addr))
            return -2; // config conflict
    }
    
    // check if conn_max >= 0
    if (conn_max < 0) return -1;

    // All checking okay, new config entry!
    // printf("ip config, %s, %ld, %d\n", ip_str, addr.sin_addr.s_addr, conn_max);
    T_IP_CHILDREN *entry = &(ip_children[ip_children_length]);
    entry->addr = addr;
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
int new_child(T_CHILD_THREAD_DATA **p, struct sockaddr_in addr) {
    T_IP_CHILDREN *entry = NULL;
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *e = &(ip_children[i]);
        if (issameipv4(&(e->addr), &addr)) {
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
    
    if (data == NULL) {
        return -2; // All alive child, already conn_max!!
    }
    
    // Write new child, need to lock (write, other might be reading)
    pthread_mutex_lock(&(entry->mutex));
    data->is_alive = 1;
    data->child_id = child_id_g;
    pthread_mutex_unlock(&(entry->mutex));
    
    child_id_g++;
    
    *p = data;
    
    print_ip_children();
    
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
struct sockaddr_in *get_child_addr(unsigned int child_id) {
    struct sockaddr_in *ret = NULL;
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *entry = &(ip_children[i]);
        pthread_mutex_lock(&(entry->mutex));
        for (int j=0; j<entry->conn_max; j++) {
            T_CHILD_THREAD_DATA *data = &(entry->child_arr[j]);
            if (child_id == data->child_id) {
                ret = &(entry->addr);
                break;
            }
        }
        pthread_mutex_unlock(&(entry->mutex));
        if (ret != NULL) break;
    }
    
    return ret;
}

/*
 * Cancel all threads
 *
 * Return:
 *         1 : Success, all child canellation requested
 *        -1 : Some child canellation failed, see errno (of one child).
 *        -2 : ip not set in configuration.
 */
int cancel_all_child(struct sockaddr_in addr) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(addr.sin_family, &(addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
    printf("%s(): Cancelling %s...\n", "cancel_all_child", ip_str);
    
    int ret = -2;
    int temp;
    for (int i=0; i<ip_children_length; i++) {
        T_IP_CHILDREN *entry = &(ip_children[i]);
        
        if (issameipv4(&(entry->addr), &addr)) {
            pthread_mutex_lock(&(entry->mutex));
            for (int j=0; j<entry->conn_max; j++) {
                T_CHILD_THREAD_DATA *data = &(entry->child_arr[j]);
                if (data->is_alive) {
                    printf("%s(): Cancelling %s, %d...\n", "cancel_all_child", ip_str, data->child_id);
                    temp = pthread_cancel(data->thr);
                    if (temp != 0) ret = -1;
                }
            }
            pthread_mutex_unlock(&(entry->mutex));
            ret = 1;
            break;
        }
    }
    
    printf("%s(): Returning %d...\n", "cancel_all_child", ret);
    return ret;
}
/* REGION END */





/* REGION START: Connection handler */
void *connectionHandler(void *arg_ptr);
void connection_cleanup_handler(void *input_ptr);
void *connectionHandler(void *input_ptr) {
    void *ret = NULL;
    T_CHILD_THREAD_DATA* child_data;

    char read_msg[STR_BUF_MAX];
    char send_msg[STR_BUF_MAX];
    
    unsigned int child_id;
    int sock;
    struct sockaddr_in *addr_client_ptr;
    char msg1[STR_BUF_MAX];
    char msg2[STR_BUF_MAX];
    char msg3[STR_BUF_MAX];
    
    
    child_data = (T_CHILD_THREAD_DATA *)input_ptr;
    child_id = child_data->child_id;
    
    printf("%s(): %d, Enter connection handler...\n", "connectionHandler", child_id);
    
    printf("%s(): %d, Read child_data...\n", "connectionHandler", child_id);
    sock = child_data->sock;
    addr_client_ptr = get_child_addr(child_id);
    strcpy(msg1, child_data->msg1);
    strcpy(msg2, child_data->msg2);
    strcpy(msg3, child_data->msg3);
    printf("%s(): %d, child_id=%d, is_alive=%d, ip=%s, sock=%d...\n", "connectionHandler", child_id,
        child_data->child_id,
        child_data->is_alive,
        inet_ntoa(addr_client_ptr->sin_addr),
        child_data->sock
    );
    
    printf("%s(): %d, Push clean up handler...\n", "connectionHandler", child_id);
    pthread_cleanup_push(connection_cleanup_handler, input_ptr);
    
    int flag_cont = 1;
    while (flag_cont) {
        /* Read query from client */
        printf("%s(): %d, Reading message from %d...\n", "connectionHandler", child_id, sock);
        int n = read_line(sock, read_msg, STR_BUF_MAX);
        if (n > 0) {
            /* Process input */
            printf("%s(): %d, Get message, n=%d, %s...\n", "connectionHandler", child_id, n, read_msg);
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
            printf("%s(): %d, Sending message, %s...\n", "connectionHandler", child_id, send_msg);
            int n = write(sock, send_msg, (strlen(send_msg) + 1) * sizeof(char));
            if (n < 0) {
                fprintf(stderr, "%s(): %d, Error on connection handler, %d, %d\n", "connectionHandler", child_id, n, errno);
                ret = (void *)(&errno);
                break;
            }
        } else if (n < 0) {
            fprintf(stderr, "%s(): %d, Error on connection handler read_line, %d, %d\n", "connectionHandler", child_id, n, errno);
            ret = (void *)(&errno);
            break;
        } else if (n == 0) {
            printf("%s(): %d, Nothing to read...\n", "connectionHandler", child_id);
            break;
        }
    }
    
    printf("%s(): %d, Pop clean up handler...\n", "connectionHandler", child_id);
    pthread_cleanup_pop(1);
    return ret;
}
void connection_cleanup_handler(void *input_ptr) {
    T_CHILD_THREAD_DATA* child_data = NULL;
    unsigned int child_id;
    int sock;

    child_data = (T_CHILD_THREAD_DATA *)input_ptr;
    child_id = child_data->child_id;
    sock = child_data->sock;

    printf("%s(): %d, %s...\n", "connection_cleanup_handler", child_id, "Clean up");
    close(sock);
    child_data->is_alive = 0;
    print_ip_children();
}
/* REGION END */




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
    printf("Read ip config...\n");
    temp = add_ip_children_config("192.168.1.3", 1);
    if (temp < 0) {
        printf("read config error: %d\n", temp);
        return 0;
    }
    
    temp = add_ip_children_config("127.0.0.1", 2);
    if (temp < 0) {
        printf("read config error: %d\n", temp);
        return 0;
    }
    
    /*
    temp = add_ip_children_config("127.0.0.1", 1);
    if (temp < 0) {
        printf("read config error: %d\n", temp);
        return 0;
    }
    */
    
    
    
    
    
    
    
    

    print_ip_children();
    
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
        temp = new_child(&data, addr_client);
        if (temp < 0) {
            /*
             *            1 : Success, you can read *p
             *           -1 : Invalid ip, the incoming ip is NOT authorized in config.
             *           -2 : Connections full. Too many alive connections for this ip. The connection maximum (being set in config) has been attained.
             */
            if (temp==-1) printf("Invalid ip, the incoming ip is NOT authorized in config\n");
            else if (temp==-2) { printf("Too many alive connections for this ip\n"); cancel_all_child(addr_client); }
            close(sock_c);
            continue;
        }
        //data->thr = child_thread;
        data->sock = sock_c;
        strcpy(data->msg1, "HELLO! This is message 1.\n");
        strcpy(data->msg2, "HELLO! This is message 2.\n");
        strcpy(data->msg3, "HELLO! This is message 3.\n");
        
        printf("Create child thread...\n");
        temp = pthread_create(&(data->thr), NULL, connectionHandler, (void *)data);
        if (temp != 0) {
            close(sock_c);
            error("Error", temp);
            break;
        }
        
        printf("Detach child thread....\n");
        temp = pthread_detach(data->thr);
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



/* Util */

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

int issameipv4(struct sockaddr_in *addr, struct sockaddr_in *addr_other) {
    if ((addr->sin_addr.s_addr == addr_other->sin_addr.s_addr)
     && (addr->sin_family == addr_other->sin_family)
     && (addr->sin_family == AF_INET)) {
        return 1;
    }
    return 0;
}
