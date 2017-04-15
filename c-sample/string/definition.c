#include <stdio.h>


int main() {
        char *temp = "hello, you can \
split the string\n";
        printf("%s", temp);

	temp = "hello, " "you can " "split the string\n";
        printf("%s", temp);

        char buf[7];
        buf[0] = 'h';
        buf[1] = 'e';
        buf[2] = 'l';
        buf[3] = 'l';
        buf[4] = 'o';
        buf[5] = '\n';
        buf[6] = '\0';

        printf("%s", buf);
	return 0;
}
