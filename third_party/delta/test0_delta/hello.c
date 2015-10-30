// We are trying to find the minimum version of this program that
// compiles but returns false.
#include <stdio.h>
int main(int argc, char **argv) {
    int a = 0;
    printf("Hello, World!\n");
    a++;
    a--;
    a+=2;
    a++;
    a--;
    a++;
    return a >= 3;} // Prevents main becoming empty.
