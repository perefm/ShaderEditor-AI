#include "catch2/catch_test_macros.hpp"

#include <iostream>

int main() {
    int failures = 0;
    for (const auto& test : Catch2Shim::registry()) {
        try {
            test.fn();
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << test.name << ": " << ex.what() << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
