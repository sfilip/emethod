#include <stdio.h>

int main(int argc, char *argv[])
{
	char line[1000];

	while (fgets(line, 1000, stdin))
	{
		printf("\"");
		for (char *p = line; *p; p++)
			switch (*p)
			{
				case '\"':
					printf("\\\"");
					break;
				case '\t':
					printf("\\t");
					break;
				case '\r':
					break;
				case '\n':
					printf("\\n");
					break;
				default:
					putchar(*p);
			}
		printf("\"\n");
	}
	return 0;
}
