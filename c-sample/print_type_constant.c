#include <stdio.h>
#include <limits.h> /* Some constants about type are stored in this header */
#include <float.h> /* Some constants about float are stored in this header */


int main() {
	/* %l: long, %u: unsigned integer, %d: integer */

	printf("Storage size for int : %lu \n", sizeof(int));
	printf("The minimum value of int : %d \n", INT_MIN);
	printf("The maximum value of int : %d \n", INT_MAX);

	printf("Storage size for float : %lu \n", sizeof(float));
	printf("The minimum value of float : %E \n", FLT_MIN);
	printf("The maximum value of float : %E \n", FLT_MAX);
	printf("The precision value of float : %d \n", FLT_DIG);

	return 0;
}
