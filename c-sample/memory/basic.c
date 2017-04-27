#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int a, b;
    int value;
    int c, d;
} F;

typedef struct {
    int x;
    F f;
    int y;
} T;

int main(){
    /*
     * Allocate a memory segment of size=sizeof(F)
     * &fsrc will point to that memory segment
     * This is a stack memory, so it will be collected after this scope has ended.
     */
    F f;
        
    /*
     * F contains 5 int fields, so it has size of 5 * sizeof(int), probably 20 (bytes)
     */
    printf("sizeof(F)=%d\n", sizeof(F));
    printf("address=[a:%p][b:%p][value:%p][c:%p][d:%p], end before:%p\n", &f.a, &f.b, &f.value, &f.c, &f.d, (&f.d + 1));
    // (&f.d + 1) will give the address of NEXT POINTER, but NOT NEXT ADDRESS!!
    
    
    /*
     * Will also allocate memory for t.f, with same lifetime
     */
    T t;
    printf("sizeof(T)=%d\n", sizeof(T));
    printf("address=[x:%p][f:%p][y:%p], end before:%p\n", &t.x, &t.f, &t.y, (&t.y + 1));
    
    /* change the value of the memory at f.value */
    f.value = 1;
    
    return(0);
}
