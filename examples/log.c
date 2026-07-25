#!/usr/bin/env cx
//DEPS gh:rxi/log.c/master/src/log
#include <stdio.h>
#include "log.h"

int main(int argc, char* argv[]) {
    // argv[1] contains the PWD where 'cx' was run from
    const char* caller_pwd = argv[1];

    log_info("Hello from cx! Executed in: %s", caller_pwd);
    log_warn("This is a warning log message.");
    log_error("And an error message!");

    return 0;
}