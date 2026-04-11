#include "editor/ShaderEditorState.h"

namespace shadereditor {
// Replacing the document wholesale is used after file loads.
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
