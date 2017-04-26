#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int id = 0;

typedef struct mytype {
    int id;
    pthread_mutex_t lock;
    
    //lock protected data
    int a;
} mytype;

void *thread_main(void *p);
/*
 * get pointer of current thread as thread id
 */
long get_tid() {
    pthread_t self_t;
    self_t = pthread_self();
    long ret = (long)&self_t;
    return ret;
}

int main() {
    long tid;
    pthread_t t;
    mytype *myobj;
    int temp;
    
    tid = get_tid();
    printf("Start Thread %ld\n", tid);
    
    printf("Init object...\n");
    myobj = malloc(sizeof(mytype));
    myobj->id = id++;
    myobj->a = 1000;
    temp = pthread_mutex_init(&(myobj->lock), NULL);
    if (temp != 0) { printf("Init mutex fail, %d\n", temp); return 1; }
    
    printf("Create thread...\n");
    temp = pthread_create(&t, NULL, thread_main, (void *)myobj);
    if (temp != 0) { printf("Create thread failed, %d\n", temp); return 1; }
    pthread_mutex_t* lock_addr = &(myobj->lock);
    
    /* Lock protected Code */
    pthread_mutex_lock(lock_addr);
    for (int i=0; i<5; i++){
        myobj->a++;
        printf("[%ld][id=%d, a=%d]\n", tid, myobj->id, myobj->a);
        //sleep(2);
    }
    pthread_mutex_unlock(lock_addr);
    
    printf("Wait other thread end...\n");
    temp = pthread_join(t, NULL);
    if (temp != 0) { printf("Join thread error, %d\n", temp); return 1; }
    
    printf("Destroy object...\n");
    temp = pthread_mutex_destroy(&(myobj->lock));
    if (temp != 0) { printf("Destroy mutex fail, %d\n", temp); return 1; }
    free(myobj);
    
    printf("Program end\n");
    return 0;
}

void *thread_main(void *p) {
    long tid = get_tid();
    mytype *myobj = (mytype *) p;
    pthread_mutex_t* lock_addr = &(myobj->lock);
    
    pthread_mutex_lock(lock_addr);
    for (int i=0; i<15; i++) {
        myobj->a++;
        printf("[%ld][id=%d, a=%d]\n", tid, myobj->id, myobj->a);
        //sleep(1);
    }
    pthread_mutex_unlock(lock_addr);
    return 0;
}
