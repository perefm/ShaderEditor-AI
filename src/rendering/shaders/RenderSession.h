#pragma once

#include "app/workspace/PlaybackClockState.h"
#include "app/workspace/PreviewInteractionState.h"
#include "rendering/shaders/UniformDefinition.h"

#include <string>
#include <unordered_map>

namespace shadereditor {
// Tracks the state of shader compilation/linking independently from per-frame rendering.
enum class ProgramStatus { Uncompiled, Compiled, Linked, Failed };
enum class FrameStatus { Idle, Rendering, Error };
// What the preview currently draws: a built-in primitive, or an Assimp-imported model.
enum class RenderTargetKind { Primitive, Model };

// Snapshot of everything the UI needs to describe and display the preview.
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
    // Which kind of geometry the preview renders; Primitive keeps today's behavior unchanged.
    RenderTargetKind renderTargetKind {RenderTargetKind::Primitive};
    // Identifier (source file path/stem) of the currently loaded model, meaningful only when
    // renderTargetKind == Model; empty if no model has ever been imported.
    std::string loadedModelId;
    // Index into the loaded model's ModelDocument::animations that SkeletalAnimator should play;
    // -1 (the default) means "no animation" (bind pose / static mesh). Ignored for primitives.
    int selectedAnimationIndex {-1};
    // Read-only snapshot of the playback clock so panels can display t/tend/bpm/beat/play-state
    // without reaching into WorkspaceController internals.
    PlaybackClockState playback;
};
}  // namespace shadereditor
