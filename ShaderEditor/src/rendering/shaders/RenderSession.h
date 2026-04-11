#pragma once

#include "app/workspace/PreviewInteractionState.h"
#include "rendering/shaders/UniformDefinition.h"

#include <string>
#include <unordered_map>

namespace shadereditor {
enum class ProgramStatus { Uncompiled, Compiled, Linked, Failed };
enum class FrameStatus { Idle, Rendering, Error };

struct RenderSession {
    std::string selectedPrimitiveId {"plane"};
    ProgramStatus programStatus {ProgramStatus::Uncompiled};
    std::unordered_map<std::string, UniformValue> uniformValues;
    FrameStatus frameStatus {FrameStatus::Idle};
    std::string errorMessage;
    std::string previewSummary;
    unsigned int previewTextureId {0};
    int previewWidth {0};
    int previewHeight {0};
    PreviewInteractionState interactionState;
};
}  // namespace shadereditor
