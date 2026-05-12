
#include <stdio.h>

int main() {
    int a = 3;
    int b = (++a * ++a); 
    {
	printf("a = %d, b = %d\n", a, b);
	}
    return 0;
}

