#!/usr/bin/env cx
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS conan:nlohmann_json/3.11.3
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False,jmespath_build_tests=False] AS jmespath::jmespath

#include <iostream>
#include <jmespath/jmespath.h>

namespace jp = jmespath;

int main() {
    auto data = R"({
        "locations": [
            {"name": "Seattle", "state": "WA"},
            {"name": "New York", "state": "NY"},
            {"name": "Bellevue", "state": "WA"},
            {"name": "Olympia", "state": "WA"}
        ]
    })"_json;

    jp::Expression expression =
        "locations[?state == 'WA'].name | sort(@) | "
        "{WashingtonCities: join(', ', @)}";

    std::cout << jp::search(expression, data) << std::endl;
    return 0;
}
