#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace shadereditor {
struct ShaderPairDocument {
    std::optional<std::filesystem::path> vertexPath;
    std::optional<std::filesystem::path> fragmentPath;
    std::string vertexSource;
    std::string fragmentSource;
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
