#pragma once

#include "editor/ShaderPairDocument.h"

#include <string>

namespace shadereditor {
// Produces user-facing messages related to document state.
class DocumentDialogs {
  public:
    [[nodiscard]] std::string unsavedChangesMessage(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
