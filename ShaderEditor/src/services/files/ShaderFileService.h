#pragma once

#include "editor/ShaderPairDocument.h"

#include <filesystem>

namespace shadereditor {
class ShaderFileService {
  public:
    ShaderPairDocument load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) const;
    std::string loadSource(const std::filesystem::path& path) const;
    void save(ShaderPairDocument& document) const;
};
}  // namespace shadereditor
