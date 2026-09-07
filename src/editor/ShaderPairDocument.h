#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace shadereditor {
struct ShaderStageLine {
    int stageLine {0};
    int documentLine {0};
};

// One Phoenix .glsl document plus the extracted sources used by OpenGL.
struct ShaderPairDocument {
    std::optional<std::filesystem::path> shaderPath;
    std::optional<std::filesystem::path> vertexPath;
    std::optional<std::filesystem::path> fragmentPath;
    std::string source;
    std::string vertexSource;
    std::string fragmentSource;
    std::vector<ShaderStageLine> vertexLineMap;
    std::vector<ShaderStageLine> fragmentLineMap;
    bool isDirty {false};
    std::chrono::system_clock::time_point lastLoadedAt {};
    std::chrono::system_clock::time_point lastSavedAt {};

    void markDirty() { isDirty = true; }
    void markSaved() {
        isDirty = false;
        lastSavedAt = std::chrono::system_clock::now();
    }
    void markLoaded() {
        isDirty = false;
        lastLoadedAt = std::chrono::system_clock::now();
    }
};
}  // namespace shadereditor

