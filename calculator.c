#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[])
{
	int x, y, z;

	scanf("%d %d %d", &x, &y, &z);

	x=y%(z/z);
	z=x+x+x+x+x+x*-(1)+x+y+y+y+y+y+y+y+y+y+y;

	printf("x = %d, y = %d, z = %d\n", x, y, z);
	return 0;
}
