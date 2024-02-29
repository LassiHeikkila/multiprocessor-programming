#include <stdio.h>
#include <stdlib.h>

#include "panic.h"

void __attribute__((noreturn)) panic(const char* desc) {
    puts("PANIC:");
    puts(desc);
    puts("exiting...");
    exit(1);
}