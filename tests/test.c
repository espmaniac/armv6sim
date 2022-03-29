// arm-linux-gnueabi-gcc -static -nostdlib -marm -march=armv6+nofp test.c
int main(void) {
	for (int i = 0; i < 10; ++i);
}
