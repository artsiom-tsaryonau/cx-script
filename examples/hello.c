#!/usr/bin/env cx
//DEPS gh:rxi/log.c/master/src/log

#include <stdio.h>
#include "log.h"

int main(void) {
    printf("Initializing C Script Runner Demo...\n\n");
    log_info("Hello, World! Everything is working smoothly.");
    return 0;
}
