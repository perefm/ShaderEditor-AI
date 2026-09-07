#include "editor/ShaderEditorState.h"

#include "services/shaders/PhoenixShaderParser.h"

#include <stdexcept>

namespace shadereditor {
void ShaderEditorState::attachDocument(ShaderPairDocument document) { document_ = std::move(document); }

void ShaderEditorState::updateSource(const std::string& source) {
    document_.source = source;
    const auto parsed = PhoenixShaderParser {}.parse(document_);
    if (!parsed.success) {
        document_.vertexSource.clear();
        document_.fragmentSource.clear();
    }
    document_.markDirty();
}

void ShaderEditorState::updateVertexSource(const std::string&) {
    throw std::logic_error("Individual vertex source editing is not supported for Phoenix documents");
}

void ShaderEditorState::updateFragmentSource(const std::string&) {
    throw std::logic_error("Individual fragment source editing is not supported for Phoenix documents");
}
}  // namespace shadereditor
