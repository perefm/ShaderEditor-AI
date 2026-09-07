#include "catch2/catch_test_macros.hpp"

#include "services/files/ShaderFileService.h"

#include <filesystem>
#include <fstream>

TEST_CASE("shader file service loads and saves shader pairs") {
    const auto temp = std::filesystem::temp_directory_path();
    const auto vertex = temp / "shader_editor_test.vert";
    const auto fragment = temp / "shader_editor_test.frag";
    {
        std::ofstream out(vertex);
        out << "void main(){}";
    }
    {
        std::ofstream out(fragment);
        out << "void main(){}";
    }

    shadereditor::ShaderFileService service;
    auto document = service.load(vertex, fragment);
    REQUIRE(document.vertexSource == "void main(){}");
    document.vertexSource = "changed";
    document.fragmentSource = "changed-frag";
    service.save(document);

    const auto reloaded = service.load(vertex, fragment);
    REQUIRE(reloaded.vertexSource == "changed");
    REQUIRE(reloaded.fragmentSource == "changed-frag");

    std::filesystem::remove(vertex);
    std::filesystem::remove(fragment);
}
