#include <stdio.h>
#include <stdlib.h>

/*
 * DOES NOT WORK. WARNING is shown. (x is local. The memory block is unreliable.);
 */
int *pointer_to_local_variable() {
    int x = NULL;
    x = 1234;
    return &x;
}
/*
 * WORK. Nothing assigned to the memory block yet.
 */
int *local_pointer_null() {
    int *x_ptr = NULL;
    return x_ptr;
}
/*
 * DOES NOT WORK...? NO WARNING is shown. Will be segmentation fault?
 */
int some_runtime_value() { int x=1233; return (x+1); }
int *local_pointer_to_runtime_value() {
    int *x_ptr = NULL;
    *xptr = some_runtime_value();
    return x_ptr;
}
/*
 * DOES NOT WORK...? Same as above
 */
int *local_pointer_to_value() {
    int *x_ptr = NULL;
    *xptr = 1234;
    return x_ptr;
}

/* So, how can we avoid the problem? */
 
/*
 * Good practice. But you MUST check for null pointer. And return state.
 */
int out_to_pointer(int *ptr) {
    if (ptr == NULL) return -1;
    x = 1234;
    *ptr = x;
}
/*
 * Good practice. But you MUST call free(ptr) after your work is done.
 */
int *local_pointer_to_heap() {
    int *xptr = NULL;
    xptr = malloc(sizeof(int));
    return xptr;
}
/*
 * Bad practice but valid.
 */
int *pointer_to_static_variable() {
    static int x = 1234; // use global memory block
    return &x;
}

int main() {
    int *ptr = NULL;
    
    /* CAN do this */
    *ptr = 1234; // implicitly allocate stack memory
    if (out_to_pointer(ptr) >= 0)
        printf("%d\n", *ptr);
    
    
    
    /* CAN do this */
    ptr = local_pointer_to_heap() ;// Re-assign pointer. The old memory block will be collected when this scope ends?
    printf("%d\n", *ptr);
    free(ptr);
    
    
    
    /* CAN do this */
    ptr = pointer_to_static_variable();
    printf("%d\n", *ptr);
    
    return 0;
}