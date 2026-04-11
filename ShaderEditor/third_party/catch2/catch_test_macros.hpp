#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Catch2Shim {
using TestFunction = std::function<void()>;
struct TestCase {
    std::string name;
    TestFunction fn;
};
inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}
struct Registrar {
    Registrar(std::string name, TestFunction fn) {
        registry().push_back({std::move(name), std::move(fn)});
    }
};
inline void require(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::ostringstream out;
        out << file << ":" << line << " requirement failed: " << expr;
        throw std::runtime_error(out.str());
    }
}
}  // namespace Catch2Shim

#define CATCH2_DETAIL_JOIN_INNER(x, y) x##y
#define CATCH2_DETAIL_JOIN(x, y) CATCH2_DETAIL_JOIN_INNER(x, y)

#define TEST_CASE(name) \
    static void CATCH2_DETAIL_JOIN(test_fn_, __LINE__)(); \
    static Catch2Shim::Registrar CATCH2_DETAIL_JOIN(test_registrar_, __LINE__)(name, CATCH2_DETAIL_JOIN(test_fn_, __LINE__)); \
    static void CATCH2_DETAIL_JOIN(test_fn_, __LINE__)()

#define REQUIRE(expr) Catch2Shim::require(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
