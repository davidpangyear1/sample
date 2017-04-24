#include <stdio.h>
#include <string.h>

int main() {
    char filename[] = "./output.txt";
    FILE *fp;
    
    /* read mode */
    fp = fopen(filename, "r");
    if (fp == NULL) { printf("\nOpen r file error.\n"); return 0; }
    int c;
    /* print file char by char */
    while ((c = getc(fp)) != EOF)
        printf("%c", c);
    fclose(fp);

    return 0;
}
