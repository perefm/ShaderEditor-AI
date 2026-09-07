#include "catch2/catch_test_macros.hpp"

#include "services/files/ShaderFileService.h"

#include <filesystem>

TEST_CASE("example shaders can be loaded from assets") {
    shadereditor::ShaderFileService service;
    const auto root = std::filesystem::path(SHADEREDITOR_SOURCE_DIR);
    const auto shader = root / "assets" / "shaders" / "basic.glsl";

    const auto document = service.load(shader);

    REQUIRE(document.shaderPath.has_value());
    REQUIRE(document.shaderPath.value() == shader);
    REQUIRE(!document.vertexSource.empty());
    REQUIRE(!document.fragmentSource.empty());
}
