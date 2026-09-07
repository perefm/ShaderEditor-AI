#pragma once

#include "editor/ShaderPairDocument.h"

namespace shadereditor {
class ShaderEditorState {
  public:
    void attachDocument(ShaderPairDocument document);
    void updateSource(const std::string& source);
    void updateVertexSource(const std::string& source);
    void updateFragmentSource(const std::string& source);
    [[nodiscard]] ShaderPairDocument& document() { return document_; }
    [[nodiscard]] const ShaderPairDocument& document() const { return document_; }

  private:
    ShaderPairDocument document_;
};
}  // namespace shadereditor

