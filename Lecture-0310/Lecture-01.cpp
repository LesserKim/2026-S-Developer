#include <iostream>
typedef signed char INT8;
typedef INT8* LPINT8;

int main()
{
	printf("'char' size id %u\n", sizeof(char));
	printf("'unsigned char' size id %u\n", sizeof(unsigned char));

	printf("'short' size id %u\n", sizeof(short));
	printf("'unsigned short' size id %u\n", sizeof(unsigned short));

	printf("'int' size id %u\n", sizeof(int));
	printf("'unsigned int' size id %u\n", sizeof(unsigned int));

	printf("'long' size id %u\n", sizeof(long));
	printf("'unsigned long' size id %u\n", sizeof(unsigned long));

	printf("'long long' size id %u\n", sizeof(long long));
	printf("'unsigned long long ' size id %u\n", sizeof(unsigned long));

	printf("'float' id %u\n", sizeof(float));
	printf("'double' id %u\n", sizeof(double));

	printf("'wchar_t' id %u\n", sizeof(wchar_t));
	printf("'INT8' size id %u\n", sizeof(INT8));
	printf("'LPINT8' size id %u\n", sizeof(LPINT8));

	return 0;
}
