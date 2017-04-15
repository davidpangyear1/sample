#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* POSIX thread library. You must also have '-lpthread' when compile. */
#include <unistd.h> /* provide the sleep function */

void *my_func(void *ptr);

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
     * function my_func     : Must accept a void*, and return a void*
     * void* arg            : As argument of my_func
     */
    printf("Create thread...\n");
    temp = pthread_create(&t, NULL, my_func, (void *)m);
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
void *my_func(void *ptr){
    printf("Entering my_func\n");
    char *message;
    message = (char *) ptr;
    for (int i = 0; i < 5; i++){
        printf("Another Thread:%s...\n", message);
        sleep(1); /* makes the current thread to sleep */
    }
    printf("my_func end\n");

    /*
     * WARNING!!!
     * You must NOT return pointer of local variable!!!
     */
    return (void *)&STATUS_A;
}
