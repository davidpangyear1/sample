#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define handle_error(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)



static void *thread_main(void *arg) {
    int temp;
    
    temp = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if (temp != 0) handle_error(temp, "pthread_setcancelstate");
    
    printf("thread_main(): started; cancellation disabled\n");
    sleep(5);
    
    printf("thread_main(): enable cancellation\n");
    
    temp = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (temp != 0) handle_error(temp, "pthread_setcancelstate");
    
    /* sleep() is a cancellation point. */
    sleep(60);
    
    /* Should never get here */
    printf("thread_main(): not cancelled\n");
    
    return NULL;
}

int main() {
    pthread_t thr;
    void* ptr;
    int temp;
    
    printf("main(): pthread_create\n");
    temp = pthread_create(&thr, NULL, &thread_main, NULL);
    if (temp != 0) handle_error(temp, "pthread_create");
    
    sleep(2); /* Allow thread getting started */
    printf("main(): pthread_cancel\n");
    temp = pthread_cancel(thr);
    if (temp != 0) handle_error(temp, "pthread_cancel");
    
    temp = pthread_join(thr, &ptr);
    if (temp != 0) handle_error(temp, "pthread_join");
    
    if (ptr == PTHREAD_CANCELED) printf("main(): thread was cancelled\n");
    else handle_error(0, "ptr == PTHREAD_CANCELED");
    
    exit(EXIT_SUCCESS);
}
