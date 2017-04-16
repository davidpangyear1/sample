#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int LENGTH = 11;

int main() {
    char str1[12] = "Hello";
    char str2[12] = "World";
    char str3[12];
    char str4[20] = "HelloWorld";
    int len;
    int n;

    /* 
     * copy at most n char. 
     * To avoid stack smashing, 
     * n must be smaller than the length of dest and src
     * Same result: strcpy(str3, str1), remember to be careful of the length
     */
    n = 5;
    strncpy(str3, str1, n);
    printf("%s\n", str3);

    /* 
     * concatenate 
     * Also be careful of array length!
     * Same result: strcat(str1, str2);
     */
    n = LENGTH - strlen(str1);
    strncat(str1, str2, n);
    printf("%s\n", str1);
    len = strlen(str1);
    printf("%d\n", len);

    /* compare */
    n = 5;
    if (strcmp("HelloWorld", str4) == 0) { printf("Same!\n"); } else { printf("Not the same? str4=%s\n",str4); }
    if (strcmp(str1, str4) == 0) { printf("Same Again!\n"); }
    if (strncmp(str1, str4, n) == 0) { printf("First %d characters same!\n", n); }

    /* find pointer of the character in str */
    char *ptr = NULL;
    ptr = strchr(str1, 'l');

    /* 
     * use memory function to edit string 
     * remember the last char must be '\0'
     */
    char *str5 = NULL;
    str5 = (char *)malloc((LENGTH + 1) * sizeof(char));
    /* 
     * copy string
     * Same result: strncpy(str5, str4, strlen(str4));
     * PS: memcpy cannot handle memory overlap, don't use it.
     */
    memmove((void *)str5, &str4, strlen(str4) + 1); /* also copy the '\0', recommended to use strncpy instead */ 
    printf("%s\n", str5);
    len = strlen(str5);
    printf("%d\n", len);

    /*
     * good way to repeat same character
     */
    n = 5;
    memset((void *)&str4[1], 'b', n);
    printf("%s\n", str4);

    return 0;
}
