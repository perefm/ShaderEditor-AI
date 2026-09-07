#pragma once

#include "rendering/models/ModelDocument.h"

#include <filesystem>
#include <string>

namespace shadereditor {
// Result of one AssimpModelLoader::load() call. On failure, document is left default-constructed
// so callers must check `success` before using `document` (never throws, per FR-019).
struct ModelLoadResult {
    bool success {false};
    std::string errorMessage;
    ModelDocument document;
};

// Imports a 3D model file via Assimp into an engine-agnostic ModelDocument, reproducing
// Phoenix's exact vertex-attribute names, texture-uniform naming, and material-color uniform
// names (see specs/004-editor-refinements-phoenix-vars-assimp/research.md section 5 and
// contracts/internal-service-contracts.md section 4). This is the only translation unit that
// includes Assimp headers, so the rest of the app never depends on Assimp types directly.
class AssimpModelLoader {
  public:
    [[nodiscard]] ModelLoadResult load(const std::filesystem::path& modelPath) const;
};
}  // namespace shadereditor
