#include <stdio.h>
#include <string.h>

int main() {
    char filename[] = "./output.txt";
    FILE *fp;
    
    /* write mode */
    fp = fopen(filename, "w");
    for (int i = 0; i < 10; i++) {
        fprintf(fp, "Hello, World!! %d\n", i);
    }
    fclose(fp);
    
    /* append mode */
    fp = fopen(filename, "a");
    for (int i = 0; i < 10; i++) {
        fprintf(fp, "Hello Again, World!! %d\n", i);
    }
    fclose(fp);
    
    return 0;
}
