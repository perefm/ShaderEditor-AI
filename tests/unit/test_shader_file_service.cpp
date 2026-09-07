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

TEST_CASE("shader file service saveAs retargets the document to a new single-file shader") {
    const auto temp = std::filesystem::temp_directory_path();
    const auto original = temp / "shader_editor_test_original.glsl";
    const auto renamed = temp / "shader_editor_test_renamed.glsl";
    {
        std::ofstream out(original);
        out << "#type vertex\nvoid mainVert(){}\n#type fragment\nvoid mainFrag(){}";
    }

    shadereditor::ShaderFileService service;
    auto document = service.load(original);
    REQUIRE(document.shaderPath.has_value());

    // saveAs() writes document.source verbatim (mirroring save()), so mutate that field directly
    // rather than the parsed vertex/fragment sub-sources.
    document.source = "#type vertex\nvoid mainVert(){}\n#type fragment\nvoid mainFrag2(){}";
    service.saveAs(document, renamed);

    // saveAs() must retarget the document to the new path and clear any split-file paths so
    // subsequent save()/load() operate on the single new file, mirroring Application's
    // "Save As" flow.
    REQUIRE(document.shaderPath.has_value());
    REQUIRE(document.shaderPath.value() == renamed);
    REQUIRE(!document.vertexPath.has_value());
    REQUIRE(!document.fragmentPath.has_value());
    REQUIRE(!document.isDirty);
    REQUIRE(std::filesystem::exists(renamed));

    const auto reloaded = service.load(renamed);
    REQUIRE(reloaded.fragmentSource.find("void mainFrag2(){}") != std::string::npos);

    std::filesystem::remove(original);
    std::filesystem::remove(renamed);
}
