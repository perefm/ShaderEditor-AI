#include "ui/panels/ModelInfoPanel.h"

#include <imgui.h>

#include <cstdio>
#include <string>

namespace shadereditor {
namespace {
void drawLabeledValue(const char* label, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextWrapped("%s", value.c_str());
}

void drawLabeledValue(const char* label, std::size_t value) {
    drawLabeledValue(label, std::to_string(value));
}

std::string formatVec3(const glm::vec3& value) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%.3f, %.3f, %.3f", value.x, value.y, value.z);
    return buffer;
}
}

ModelInfoPanel::ModelInfoPanel(WorkspaceController& controller) : controller_(controller) {}

void ModelInfoPanel::draw() const {
    const ModelInfoSummary& info = controller_.modelInfo();
    if (!info.hasModel) {
        ImGui::TextWrapped("No model loaded. Use File > Open model... to import one.");
        return;
    }

    constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg;

    if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##model-info-source", 2, tableFlags)) {
            drawLabeledValue("Name", info.name);
            drawLabeledValue("Path", info.sourcePath);
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##model-info-geometry", 2, tableFlags)) {
            drawLabeledValue("Meshes", info.meshCount);
            drawLabeledValue("Vertices", info.vertexCount);
            drawLabeledValue("Triangles", info.triangleCount);
            drawLabeledValue("Indices", info.indexCount);
            drawLabeledValue("Materials", info.materialCount);
            drawLabeledValue("Scene nodes", info.sceneNodeCount);
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##model-info-textures", 2, tableFlags)) {
            drawLabeledValue("Has textures", std::string(info.hasTextures ? "yes" : "no"));
            drawLabeledValue("Texture slots", info.textureCount);
            drawLabeledValue("Embedded textures", info.embeddedTextureCount);
            for (const auto& [type, count] : info.texturesByType) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(("  " + type).c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(std::to_string(count).c_str());
            }
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Skeleton & animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##model-info-skeleton", 2, tableFlags)) {
            drawLabeledValue("Has skeleton", std::string(info.hasSkeleton ? "yes" : "no"));
            drawLabeledValue("Bones", info.boneCount);
            drawLabeledValue("Animation clips", info.animations.size());
            ImGui::EndTable();
        }
        if (!info.animations.empty() && ImGui::BeginTable("##model-info-animations", 3, tableFlags | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Clip");
            ImGui::TableSetupColumn("Duration (s)");
            ImGui::TableSetupColumn("Channels");
            ImGui::TableHeadersRow();
            for (const auto& clip : info.animations) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(clip.name.empty() ? "(unnamed)" : clip.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", clip.durationSeconds);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(std::to_string(clip.channelCount).c_str());
            }
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Cameras", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (info.cameras.empty()) {
            ImGui::TextDisabled("This model has no cameras.");
        } else if (ImGui::BeginTable("##model-info-cameras", 2, tableFlags | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Camera");
            ImGui::TableSetupColumn("Animated");
            ImGui::TableHeadersRow();
            for (const auto& camera : info.cameras) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(camera.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(camera.animated ? "yes" : "no");
            }
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##model-info-bounds", 2, tableFlags)) {
            drawLabeledValue("Min", formatVec3(info.boundsMin));
            drawLabeledValue("Max", formatVec3(info.boundsMax));
            drawLabeledValue("Size", formatVec3(info.boundsSize));
            drawLabeledValue("Bounding radius", std::to_string(info.boundingRadius));
            ImGui::EndTable();
        }
    }
}
}  // namespace shadereditor
