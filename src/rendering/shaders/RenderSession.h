#pragma once

#include "app/workspace/PlaybackClockState.h"
#include "app/workspace/PreviewInteractionState.h"
#include "rendering/shaders/UniformDefinition.h"

#include <glm/vec4.hpp>

#include <cmath>
#include <string>
#include <unordered_map>

namespace shadereditor {
// Tracks the state of shader compilation/linking independently from per-frame rendering.
enum class ProgramStatus { Uncompiled, Compiled, Linked, Failed };
enum class FrameStatus { Idle, Rendering, Error };
// What the preview currently draws: a built-in primitive, or an Assimp-imported model.
enum class RenderTargetKind { Primitive, Model };

struct ViewportMetrics {
    int widthPixels {1};
    int heightPixels {1};
    float aspectRatio {1.0F};
};

inline ViewportMetrics makeViewportMetrics(int width, int height) {
    const int safeWidth = width > 0 ? width : 1;
    const int safeHeight = height > 0 ? height : 1;
    return ViewportMetrics {
        safeWidth,
        safeHeight,
        static_cast<float>(safeWidth) / static_cast<float>(safeHeight),
    };
}

class RenderMetrics {
  public:
    void recordFrame(float deltaSeconds) {
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) {
            return;
        }
        accumulatedSeconds_ += deltaSeconds;
        ++accumulatedFrames_;
        if (accumulatedSeconds_ >= kUpdateIntervalSeconds) {
            displayFps_ = static_cast<float>(accumulatedFrames_) / accumulatedSeconds_;
            accumulatedSeconds_ = 0.0F;
            accumulatedFrames_ = 0;
        } else if (displayFps_ == 0.0F) {
            displayFps_ = 1.0F / deltaSeconds;
        }
        if (!std::isfinite(displayFps_) || displayFps_ < 0.0F) {
            displayFps_ = 0.0F;
        }
    }

    [[nodiscard]] float displayFps() const { return displayFps_; }

  private:
    static constexpr float kUpdateIntervalSeconds {0.5F};
    float accumulatedSeconds_ {0.0F};
    int accumulatedFrames_ {0};
    float displayFps_ {0.0F};
};

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
    ViewportMetrics viewport;
    float displayFps {0.0F};
    glm::vec4 backgroundColor {0.09F, 0.10F, 0.13F, 1.0F};
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
