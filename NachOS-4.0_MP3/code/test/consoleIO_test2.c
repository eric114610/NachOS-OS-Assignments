#include "syscall.h"
int a[1000] = {0};
const int x=831;
const char* readOnlyString = "Hello kmkls!";
const int readOnlyArray[] = {111,112,113,114,115};
int main() {
	int n;
	for (n=15; n<=19; n++) {
		PrintInt(n);
	}
	PrintInt(x);
	PrintInt(readOnlyArray[1]);
	a[600] = 11111;
	PrintInt(a[600]);
	return 0;
	// Halt();
}

