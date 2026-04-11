#include "editor/ShaderEditorState.h"

namespace shadereditor {
void ShaderEditorState::attachDocument(ShaderPairDocument document) { document_ = std::move(document); }

void ShaderEditorState::updateVertexSource(const std::string& source) {
    document_.vertexSource = source;
    document_.markDirty();
}

void ShaderEditorState::updateFragmentSource(const std::string& source) {
    document_.fragmentSource = source;
    document_.markDirty();
}
}  // namespace shadereditor
