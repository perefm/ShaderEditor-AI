#include "services/files/ShaderFileService.h"

#include "services/shaders/PhoenixShaderParser.h"

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

ShaderPairDocument ShaderFileService::load(const std::filesystem::path& shaderPath) const {
    ShaderPairDocument document;
    document.shaderPath = shaderPath;
    document.source = readFile(shaderPath);
    const auto parsed = PhoenixShaderParser {}.parse(document);
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage);
    }
    document.markLoaded();
    return document;
}

ShaderPairDocument ShaderFileService::load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) const {
    ShaderPairDocument document;
    document.vertexPath = vertexPath;
    document.fragmentPath = fragmentPath;
    document.source = "#type vertex\n" + readFile(vertexPath) + "\n#type fragment\n" + readFile(fragmentPath);
    const auto parsed = PhoenixShaderParser {}.parse(document);
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage);
    }
    if (!document.vertexSource.empty() && document.vertexSource.back() == 10) {
        document.vertexSource.pop_back();
    }
    if (!document.fragmentSource.empty() && document.fragmentSource.back() == 10) {
        document.fragmentSource.pop_back();
    }
    document.markLoaded();
    return document;
}

std::string ShaderFileService::loadSource(const std::filesystem::path& path) const { return readFile(path); }

void ShaderFileService::save(ShaderPairDocument& document) const {
    if (document.shaderPath) {
        std::ofstream output(*document.shaderPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to save shader: " + document.shaderPath->string());
        }
        output << document.source;
        document.markSaved();
        return;
    }
    if (document.vertexPath && document.fragmentPath) {
        std::ofstream vertexOut(*document.vertexPath, std::ios::binary | std::ios::trunc);
        std::ofstream fragmentOut(*document.fragmentPath, std::ios::binary | std::ios::trunc);
        if (!vertexOut || !fragmentOut) {
            throw std::runtime_error("Unable to save legacy shader paths");
        }
        vertexOut << document.vertexSource;
        fragmentOut << document.fragmentSource;
        document.markSaved();
        return;
    }
    throw std::runtime_error("Cannot save shader without a .glsl file path");
}
}  // namespace shadereditor

