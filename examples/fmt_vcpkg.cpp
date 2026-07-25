#!/usr/bin/env cx
//DEPS vcpkg:fmt

#include <fmt/core.h>

int main() {
    fmt::print("Hello from cx + vcpkg ({})!\n", "fmt");
    return 0;
}
