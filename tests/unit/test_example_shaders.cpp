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

// Spec 008: the mega shader and its two stylized siblings must parse the same way any other
// bundled shader does (#type vertex/#type fragment split by ShaderFileService).
TEST_CASE("mega material shader can be loaded from assets") {
    shadereditor::ShaderFileService service;
    const auto root = std::filesystem::path(SHADEREDITOR_SOURCE_DIR);
    const auto shader = root / "assets" / "shaders" / "mega_material.glsl";

    const auto document = service.load(shader);

    REQUIRE(document.shaderPath.has_value());
    REQUIRE(document.shaderPath.value() == shader);
    REQUIRE(!document.vertexSource.empty());
    REQUIRE(!document.fragmentSource.empty());
}

TEST_CASE("toon material shader can be loaded from assets") {
    shadereditor::ShaderFileService service;
    const auto root = std::filesystem::path(SHADEREDITOR_SOURCE_DIR);
    const auto shader = root / "assets" / "shaders" / "toon_material.glsl";

    const auto document = service.load(shader);

    REQUIRE(document.shaderPath.has_value());
    REQUIRE(document.shaderPath.value() == shader);
    REQUIRE(!document.vertexSource.empty());
    REQUIRE(!document.fragmentSource.empty());
}

TEST_CASE("rim lighting material shader can be loaded from assets") {
    shadereditor::ShaderFileService service;
    const auto root = std::filesystem::path(SHADEREDITOR_SOURCE_DIR);
    const auto shader = root / "assets" / "shaders" / "rim_lighting_material.glsl";

    const auto document = service.load(shader);

    REQUIRE(document.shaderPath.has_value());
    REQUIRE(document.shaderPath.value() == shader);
    REQUIRE(!document.vertexSource.empty());
    REQUIRE(!document.fragmentSource.empty());
}
