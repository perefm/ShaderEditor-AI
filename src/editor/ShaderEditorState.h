#pragma once

#include "editor/ShaderPairDocument.h"

namespace shadereditor {
// Owns the active vertex/fragment document being edited.
class ShaderEditorState {
  public:
    void attachDocument(ShaderPairDocument document);
    void updateVertexSource(const std::string& source);
    void updateFragmentSource(const std::string& source);
    [[nodiscard]] ShaderPairDocument& document() { return document_; }
    [[nodiscard]] const ShaderPairDocument& document() const { return document_; }

  private:
    ShaderPairDocument document_;
};
}  // namespace shadereditor
