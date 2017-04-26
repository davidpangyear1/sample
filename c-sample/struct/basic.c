#include <stdio.h>
#include <string.h>

int BOOK_ID = 100000;

struct Books {
    char title[100];
    char author[50];
    int book_id;
};

void print_info(struct Books b);
void make_my_book_fail(struct Books *ptr);
void make_my_book(struct Books **ptr);

int main() {
    /*
     * Just a quick way to define bunch of variables~~
     */
    struct Books book1;
    strcpy( book1.title, "C Programming");
    strcpy ( book1.author, "Somebody");
    book1.book_id = (++BOOK_ID);
    print_info(book1);

    /*
     * Will fail
     */
    struct Books book2;
    make_my_book_fail(&book2);
    print_info(book2);

    /* Pass (Books **) is much better */
    struct Books *book2_ptr;
    make_my_book(&book2_ptr);
    print_info(*book2_ptr);
    
    return 0;
}

void print_info(struct Books b) {
    printf( "%d:%s - %s\n", b.book_id, b.title, b.author);
}

void make_my_book_fail(struct Books *ptr) {
    /*
     * Error!!! The address of b is NOT ptr!!
     */
    struct Books b = *ptr;
    strcpy(b.title, "Hahaha");
    strcpy(b.author, "Me");
    b.book_id = (++BOOK_ID);
    
    // print_info(b);
}

void make_my_book(struct Books **ptr) {
    struct Books b;
    strcpy(b.title, "Hahaha");
    strcpy(b.author, "Me");
    b.book_id = (++BOOK_ID);
    
    *ptr = &b;
}
