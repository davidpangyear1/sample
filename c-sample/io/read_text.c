#include <stdio.h>
#include <string.h>

#define BUFFER_MAX 16

/*
 * forward file pointer after c, return -1 if it meets EOF, otherwise 1
 */
int forward2char(FILE* fp, char c);

int main() {
    char filename[] = "./output.txt";
    FILE *fp;
    int temp;
    
    
    
    /* ==================== read simply ==================== */
    fp = fopen(filename, "r");
    if (fp == NULL) { printf("\nOpen r file error.\n"); return 0; }
    /* print file char by char */
    while ((temp = getc(fp)) != EOF)
        printf("%c", temp);
    fclose(fp);

    
    
    
    /* ==================== read file line by line ==================== */
    /* don't expect the file contains '\0' */
    char buf[BUFFER_MAX];
    fp = fopen(filename, "r");
    if (fp == NULL) { printf("\nOpen r file error.\n"); return 0; }
    int lc = 0; // line count
    int llen = 0; // line length read, also the next buffer index
    int cont = 1;
    while (cont) {
        // always reserve two slots before read
        if (llen + 1 >= BUFFER_MAX) {
            printf("Not enough buffer for the line, forward file pointer and ignore. line index(zero-based): %d\n", lc);
            if (forward2char(fp, '\n') < 0) break;
            llen = 0;
            lc++;
            continue;
        }
        
        temp = getc(fp);
        if (temp != '\n' && temp != EOF) {
            // buffer char and continue
            buf[llen] = temp;
            llen++;
            continue;
        }
        
        // ready for handling the line
        if (temp == EOF) {
            buf[llen] = '\0';
            // mark as last line
            cont = 0;
        } else if (temp == '\n') {
            buf[llen] = '\n';
            llen++;
            buf[llen] = '\0';
        }
        
        // handle the line
        printf("%s", buf);
        
        // reset buffer index
        llen = 0;
        lc++;
    }
    
    
    return 0;
}

int forward2char(FILE* fp, char c) {
    int temp;
    while (1) {
        temp = getc(fp);
        if (temp == EOF) {
            return -1;
        } else if (temp == c) {
            return 1;
        }
    }
}
