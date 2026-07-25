#!/usr/bin/env cx
//DEPS conan:nlohmann_json/3.11.3
//DEPS vcpkg:fmt

#include <fmt/core.h>
#include <nlohmann/json.hpp>

int main() {
    nlohmann::json j = {{"pm", "conan"}, {"lib", "nlohmann_json"}};
    fmt::print("mixed: {} + vcpkg:fmt → {}\n", j.dump(), "ok");
    return 0;
}
