/* Definitions
 *
 * // volatile integer
 * volatile int x;
 * int volatile x;
 *
 * // pointer to a volatile integer
 * volatile int *p;
 * int volatile *p;
 *
 * // volatile pointer to an integer
 * int *volatile p;
 *
 * // volatile pointer to a volatile integer
 * int volatile *volatile p;
 */
 
 /*
  * A volatile struct/union will have all its contents volatile
  */
  
  
  
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* POSIX thread library. You must also have '-lpthread' when compile. */
#include <unistd.h> /* provide the sleep function */

void *thread1(void *ptr);

/*
 * If no 'volatile', the compiler might optimize the code and make the program fail.
 * Try it!
 */
volatile int val_g;

int main() {
    pthread_t t1;
    pthread_t t2;
    const char *m = "hello";
    
    int temp;
    void *ret_void_addr;
    
    
    printf("Create thread t1...\n");
    temp = pthread_create(&t1, NULL, thread1, (void *)m);
    if (temp != 0){
        fprintf(stderr, "Error on p_thread_create, %d\n", temp);
        exit(EXIT_FAILURE);
    }
    
    sleep(7);
    val_g++;
    sleep(3);
    
    printf("Calling thread_join t1...\n");
    temp = pthread_join(t1, &ret_void_addr);
    if (temp != 0){
        fprintf(stderr, "Error on p_thread_join, %d\n", temp);
        exit(EXIT_FAILURE);
    }
    
    printf("Result:%d\n", val_g);
    
    printf("Success!\n");

    exit(EXIT_SUCCESS);
}

void *thread1(void *ptr) {    
    /* 
     * After optimizing (without volatile),
     *     val_g = 0;
     *     while (!val_g)
     * becomes
     *     while (1)
     */
    val_g = 0;
    while (!val_g) {
        printf("thread1...\n");
        sleep(1);
    }
    
    return NULL;
}
