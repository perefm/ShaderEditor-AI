#include "services/files/ShaderFileService.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace shadereditor {
namespace {
std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open shader file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

ShaderPairDocument ShaderFileService::load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) const {
    ShaderPairDocument document;
    document.vertexPath = vertexPath;
    document.fragmentPath = fragmentPath;
    document.vertexSource = readFile(vertexPath);
    document.fragmentSource = readFile(fragmentPath);
    document.markLoaded();
    return document;
}

std::string ShaderFileService::loadSource(const std::filesystem::path& path) const { return readFile(path); }

void ShaderFileService::save(ShaderPairDocument& document) const {
    if (!document.vertexPath || !document.fragmentPath) {
        throw std::runtime_error("Cannot save shaders without both file paths");
    }

    std::ofstream vertexOut(*document.vertexPath, std::ios::binary | std::ios::trunc);
    if (!vertexOut) {
        throw std::runtime_error("Unable to save vertex shader: " + document.vertexPath->string());
    }
    vertexOut << document.vertexSource;

    std::ofstream fragmentOut(*document.fragmentPath, std::ios::binary | std::ios::trunc);
    if (!fragmentOut) {
        throw std::runtime_error("Unable to save fragment shader: " + document.fragmentPath->string());
    }
    fragmentOut << document.fragmentSource;
    document.markSaved();
}
}  // namespace shadereditor
