#include <stdint.h>

int64_t add_numbers(int64_t a, int64_t b) {
    return a + b;
}

//  gcc -shared -o libmath.dll -fPIC mathlib.c
char* get_name(int len) {
    return  "Hello World";
}