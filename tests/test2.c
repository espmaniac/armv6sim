// arm-none-eabi-gcc -static -nostartfiles  -marm -march=armv6+nofp ./test2.c -L /usr/local/lib/ -lm

#include <math.h>

static unsigned test = 0;

int main() {
    test = 10;
    for (unsigned char i = 1; i < 5; i++)
        test += pow(2, i);
    return 0;
}