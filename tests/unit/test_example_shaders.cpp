#include "catch2/catch_test_macros.hpp"

#include "services/files/ShaderFileService.h"

#include <filesystem>

TEST_CASE("example shaders can be loaded from assets") {
    shadereditor::ShaderFileService service;
    const auto root = std::filesystem::path(SHADEREDITOR_SOURCE_DIR);
    const auto vertex = root / "assets" / "shaders" / "basic.vert";
    const auto fragment = root / "assets" / "shaders" / "basic.frag";

    const auto document = service.load(vertex, fragment);

    REQUIRE(document.vertexPath == vertex);
    REQUIRE(document.fragmentPath == fragment);
    REQUIRE(!document.vertexSource.empty());
    REQUIRE(!document.fragmentSource.empty());
}
