/*
For more information about C Preprocessor, see:
https://gcc.gnu.org/onlinedocs/cpp/index.html
*/

#include <stdio.h>

/* Define constants */
#define PI 3.14159

/* Define macros */
#define min(X, Y) ((X) < (Y) ? (X) : (Y))

/* Use backslash '\' to escape the newline character. */
/* # converts parameters into strings */
#define message_for(a, b) \
	printf(#a " and " #b ": We hate you!\n")

int main() {

	printf("hello, PI=%f \n", PI);

	#undef PI

	#ifndef PI	/* same as #if !defined (PI) */
		#define PIE 3.1416
	#endif

	#if defined (PI)
		printf("hello, PI=%f \n", PI);
	#elif defined (PIE)
		printf("hello, PIE=%f \n", PIE);
	#else
		printf("hello, PI and PIE are not defined!");
	#endif


	printf("hi, __FILE__=%s\n", __FILE__ );
	printf("hi, __DATE__=%s\n", __DATE__ );
	printf("hi, __TIME__=%s\n", __TIME__ );
	printf("hi, __LINE__=%d\n", __LINE__ ); /* Current line number */
	printf("hi, __STDC__=%d\n", __STDC__ );	/* Defined as 1 when the compiler complies with the ANSI standard. */

	printf("hi, the smaller is %d\n", min(2,5));

	message_for(Amy, Betty);

	return 0;
}
