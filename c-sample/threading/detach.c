#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* POSIX thread library. You must also have '-lpthread' when compile. */
#include <unistd.h> /* unix standard library, provide the sleep function */

void *thread_main(void *ptr);

int main() {
    pthread_t t;
    const char *m = "hello";
    int temp;
    
    /* create thread */
    printf("Create thread...\n");
    temp = pthread_create(&t, NULL, thread_main, (void *)m);
    if (temp != 0){
        fprintf(stderr, "Error on pthread_create, %d\n", temp);
        exit(EXIT_FAILURE);
    }
    
    /*
     * pthread_detach a thread, such that after it is teminated, the resources of that thread will be auto-released.
     * It is safe to call detach even if that thread has been terminated
     * WARNING: DON'T call it again; DON'T call pthread_join;
     */
    printf("Calling thread_detach...\n");
    temp = pthread_detach(t);
    if (temp != 0){
        fprintf(stderr, "Error on pthread_detach, %d\n", temp);
        /*
         * EINVAL : thread is not a joinable thread.
         * ESRCH  : No thread with the ID thread could be found.
         */
        exit(EXIT_FAILURE);
    }

    /* do main thread things...*/
    for (int i = 0; i < 5; i++){
        printf("Main thread...\n");
        sleep(2);
    }
    
    printf("Success!\n");
    return 0;
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
        //sleep(1); /* makes the current thread to sleep */
    }
    printf("thread_main end\n");

    return NULL;
}
