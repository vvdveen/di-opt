#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("MAIN!\n");
}
static void init(void) __attribute__ ((constructor));
static void init(void) {
    printf("INIT!\n");
}
