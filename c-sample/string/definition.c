#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main() {
        char *temp = "hello, you can \
split the string\n";
        printf("%s", temp);

	temp = "hello, " "you can " "split the string\n";
        printf("%s", temp);

        char greeting[6] = {'H', 'e', 'l', 'l', 'o', '\0'};
        printf("%s\n", greeting);

        char buf[6];
        buf[0] = 'h';
        buf[1] = 'e';
        buf[2] = 'l';
        buf[3] = 'l';
        buf[4] = 'o';
        buf[5] = '\0';
        printf("%s\n", buf);

        /* this way can define dynamic length */
        char *ptr = NULL;
        ptr = malloc((5 + 1) * sizeof(char));
        strncpy(ptr, "hello", 5);
        printf("%s\n", ptr);

	return 0;
}
