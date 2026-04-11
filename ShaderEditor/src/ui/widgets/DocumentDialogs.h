#pragma once

#include "editor/ShaderPairDocument.h"

#include <string>

namespace shadereditor {
class DocumentDialogs {
  public:
    [[nodiscard]] std::string unsavedChangesMessage(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
