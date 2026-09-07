#pragma once

#include "editor/ShaderPairDocument.h"

#include <filesystem>

namespace shadereditor {
class ShaderFileService {
  public:
    ShaderPairDocument load(const std::filesystem::path& shaderPath) const;
    ShaderPairDocument load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) const;
    std::string loadSource(const std::filesystem::path& path) const;
    void save(ShaderPairDocument& document) const;
    // Writes the document's current source to a brand-new file path and re-targets the
    // document to that path (as a single-file Phoenix shader), so subsequent Save/Update
    // calls operate on the new location. Mirrors save()'s failure handling (throws on I/O error).
    void saveAs(ShaderPairDocument& document, const std::filesystem::path& newPath) const;
};
}  // namespace shadereditor
