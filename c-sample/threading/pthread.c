#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* POSIX thread library. You must also have '-lpthread' when compile. */
#include <unistd.h> /* unix standard library, provide the sleep function */

void *thread_main(void *ptr);
long get_tid();

int STATUS_A = 1;
int STATUS_B = 2;

int main() {
    pthread_t t;
    const char *m = "hello";
    int temp;
    void *ret_void_addr;
    int *ret_addr;
    int ret;

    /* 
     * pthread_t* t         : Thread id is stored as &t.
     * pthread_attr_t* attr : Just set NULL
     * function thread_main : Must accept a void*, and return a void*
     * void* arg            : As argument of thread_main
     */
    printf("Create thread...\n");
    temp = pthread_create(&t, NULL, thread_main, (void *)m);
    if (temp != 0){
        fprintf(stderr, "Error on p_thread_create, %d\n", temp);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; i++){
        printf("Main thread...\n");
        sleep(2);
    }

    /*
     * pthread_join can release resources of a thread.
     * It will also block the current thread until t exit.
     */
    printf("Calling thread_join...\n");
    temp = pthread_join(t, &ret_void_addr);
    if (temp != 0){
        fprintf(stderr, "Error on p_thread_join, %d\n", temp);
        exit(EXIT_FAILURE);
    }

    printf("Print result...\n");
    ret_addr = (int *)ret_void_addr;
    ret = *ret_addr;
    printf("%d\n", ret);
    printf("Success!\n");

    exit(EXIT_SUCCESS);
}

/*
 * function to be called by new thread, must accept a void* and return a void*
 */
void *thread_main(void *ptr){
    printf("Entering thread_main\n");
    char *message;
    message = (char *) ptr;
    for (int i = 0; i < 5; i++){
        printf("Another Thread:%s...\n", message);
        sleep(1); /* makes the current thread to sleep */
    }
    printf("thread_main end\n");

    /*
     * WARNING!!!
     * You must NOT return pointer of local variable!!!
     */
    return (void *)&STATUS_A;
}

/*
 * get pointer of current thread as thread id
 */
long get_tid() {
    pthread_t self_t;
    self_t = pthread_self();
    long ret = (long)&self_t;
    return ret;
}
