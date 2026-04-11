#include "app/application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

#include <algorithm>
#include <array>
#include <cfloat>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace shadereditor {
namespace {
constexpr ImGuiInputTextFlags kShaderEditorFlags = ImGuiInputTextFlags_AllowTabInput;

bool hasPanel(const std::vector<std::string>& panels, const char* panelId) {
    return std::find(panels.begin(), panels.end(), panelId) != panels.end();
}

bool editMultilineString(const char* label, std::string& value, float height) {
    std::vector<char> buffer(value.begin(), value.end());
    buffer.resize(std::max<std::size_t>(buffer.size() + 4096, 4096), '\0');
    const bool changed = ImGui::InputTextMultiline(label, buffer.data(), buffer.size(), ImVec2(-FLT_MIN, height), kShaderEditorFlags);
    if (changed) {
        value.assign(buffer.data());
    }
    return changed;
}

#ifdef _WIN32
std::optional<std::filesystem::path> openShaderPathDialog(const char* title, const char* filter) {
    char pathBuffer[MAX_PATH] = {};
    OPENFILENAMEA dialog {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrTitle = title;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = pathBuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameA(&dialog)) {
        return std::nullopt;
    }
    return std::filesystem::path(pathBuffer);
}
#endif
}

Application::Application()
    : workspace_(diagnostics_),
      shaderEditorPanel_(workspace_),
      renderViewPanel_(workspace_),
      uniformsPanel_(workspace_),
      diagnosticsPanel_(diagnostics_),
      shaderErrorsPanel_(diagnostics_) {}

bool Application::initialize() {
    if (!windowContext_.initialize()) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#else
    diagnostics_.addError("Installed ImGui package does not expose docking support. Reinstall imgui with the vcpkg feature 'docking-experimental'.");
#endif
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windowContext_.window(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    registerPanels();
    exampleVertexPath_ = std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "shaders" / "basic.vert";
    exampleFragmentPath_ = std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "shaders" / "basic.frag";
    const auto layoutPath = std::filesystem::path("build") / "layout.txt";
    if (std::filesystem::exists(layoutPath)) {
        layoutState_ = layoutPersistence_.load(layoutPath);
    } else {
        layoutState_.setOpenPanels({"shader-editor", "render-view", "uniforms", "diagnostics", "shader-errors"});
        layoutState_.setFocusedPanel("shader-editor");
    }
    restorePanelVisibility();
    loadExampleShaders();
    diagnostics_.addInfo("Application initialized.");
    return true;
}

int Application::run() {
    diagnostics_.addInfo("Using backend: " + windowContext_.backend());
    diagnostics_.addInfo("Dockable panels registered: " + std::to_string(layoutState_.openPanels().size()));
    diagnostics_.addInfo("Application running.");

    while (!glfwWindowShouldClose(windowContext_.window())) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawUi();

        ImGui::Render();
        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(windowContext_.window(), &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.08F, 0.08F, 0.10F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(windowContext_.window());
    }

    return 0;
}

void Application::shutdown() {
    std::filesystem::create_directories("build");
    storePanelVisibility();
    layoutPersistence_.save(layoutState_, std::filesystem::path("build") / "layout.txt");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    windowContext_.shutdown();
    diagnostics_.addInfo("Application shutdown.");
}

void Application::registerPanels() {
    dockspaceHost_.registerPanel("shader-editor");
    dockspaceHost_.registerPanel("render-view");
    dockspaceHost_.registerPanel("uniforms");
    dockspaceHost_.registerPanel("diagnostics");
    dockspaceHost_.registerPanel("shader-errors");
}

void Application::drawUi() {
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
        shaderEditorPanel_.pressCtrlEnter();
    }

    drawWorkspaceHost();
    drawShaderEditorWindow();
    drawRenderViewWindow();
    drawUniformsWindow();
    drawDiagnosticsWindow();
    drawShaderErrorsWindow();
}

bool Application::loadExampleShaders() {
    if (!std::filesystem::exists(exampleVertexPath_) || !std::filesystem::exists(exampleFragmentPath_)) {
        diagnostics_.addError("Example shaders are missing from assets/shaders.");
        return false;
    }

    const bool loaded = workspace_.openShaders(exampleVertexPath_, exampleFragmentPath_);
    if (loaded) {
        diagnostics_.addInfo("Loaded example shaders from " + exampleVertexPath_.parent_path().string() + ".");
    }
    return loaded;
}

bool Application::openVertexShaderFromDialog() {
#ifdef _WIN32
    const auto selected =
        openShaderPathDialog("Open Vertex Shader", "Vertex Shader (*.vert;*.vs)\0*.vert;*.vs\0GLSL Files (*.glsl)\0*.glsl\0All Files (*.*)\0*.*\0");
    return selected ? workspace_.openVertexShader(*selected) : false;
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

bool Application::openFragmentShaderFromDialog() {
#ifdef _WIN32
    const auto selected =
        openShaderPathDialog("Open Fragment Shader", "Fragment Shader (*.frag;*.fs)\0*.frag;*.fs\0GLSL Files (*.glsl)\0*.glsl\0All Files (*.*)\0*.*\0");
    return selected ? workspace_.openFragmentShader(*selected) : false;
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

void Application::restorePanelVisibility() {
    const auto& openPanels = layoutState_.openPanels();
    showShaderEditor_ = hasPanel(openPanels, "shader-editor");
    showRenderView_ = hasPanel(openPanels, "render-view");
    showUniforms_ = hasPanel(openPanels, "uniforms");
    showDiagnostics_ = hasPanel(openPanels, "diagnostics");
    showShaderErrors_ = hasPanel(openPanels, "shader-errors");
}

void Application::storePanelVisibility() {
    std::vector<std::string> openPanels;
    if (showShaderEditor_) {
        openPanels.emplace_back("shader-editor");
    }
    if (showRenderView_) {
        openPanels.emplace_back("render-view");
    }
    if (showUniforms_) {
        openPanels.emplace_back("uniforms");
    }
    if (showDiagnostics_) {
        openPanels.emplace_back("diagnostics");
    }
    if (showShaderErrors_) {
        openPanels.emplace_back("shader-errors");
    }

    layoutState_.setOpenPanels(std::move(openPanels));
}

void Application::drawMainMenu() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Load Example Shaders")) {
            loadExampleShaders();
        }
        if (ImGui::MenuItem("Open Vertex Shader...")) {
            openVertexShaderFromDialog();
        }
        if (ImGui::MenuItem("Open Fragment Shader...")) {
            openFragmentShaderFromDialog();
        }
        if (ImGui::MenuItem("Save Current Shaders", "Ctrl+S")) {
            workspace_.saveShaders();
        }
        if (ImGui::MenuItem("Update Shaders", "Ctrl+Enter")) {
            shaderEditorPanel_.pressUpdateButton();
        }
        if (ImGui::MenuItem("Exit")) {
            glfwSetWindowShouldClose(windowContext_.window(), GLFW_TRUE);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Shader Editor", nullptr, &showShaderEditor_);
        ImGui::MenuItem("Render View", nullptr, &showRenderView_);
        ImGui::MenuItem("Uniforms", nullptr, &showUniforms_);
        ImGui::MenuItem("Diagnostics", nullptr, &showDiagnostics_);
        ImGui::MenuItem("Shader Errors", nullptr, &showShaderErrors_);
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void Application::drawWorkspaceHost() {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
    ImGui::SetNextWindowSize(io.DisplaySize);

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::Begin("Workspace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    drawMainMenu();
    ImGui::TextUnformatted("GLFW + OpenGL + ImGui runtime active");
    ImGui::SameLine();
#ifdef IMGUI_HAS_DOCK
    ImGui::TextUnformatted("| Docking enabled");
    // The workspace host owns the single dockspace so every major tool panel can be rearranged safely.
    ImGui::DockSpace(ImGui::GetID("WorkspaceDockspace"), ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
#else
    ImGui::TextUnformatted("| Docking unavailable in current ImGui build");
#endif
    ImGui::End();
}

void Application::drawShaderEditorWindow() {
    if (!showShaderEditor_) {
        return;
    }

    auto& editorState = workspace_.editorState();
    const auto& document = editorState.document();
    if (ImGui::Begin("Shader Editor", &showShaderEditor_)) {
        ImGui::TextUnformatted("Vertex + Fragment shader editor");
        ImGui::SameLine();
        if (document.isDirty) {
            ImGui::TextUnformatted("(modified)");
        }

        if (ImGui::Button("Load Example")) {
            loadExampleShaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Vertex")) {
            openVertexShaderFromDialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Fragment")) {
            openFragmentShaderFromDialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            workspace_.saveShaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Update Shader")) {
            shaderEditorPanel_.pressUpdateButton();
        }

        if (document.isDirty) {
            ImGui::SameLine();
            ImGui::TextWrapped("%s", documentDialogs_.unsavedChangesMessage(document).c_str());
        }

        std::string vertexSource = document.vertexSource;
        ImGui::SeparatorText("Vertex");
        if (editMultilineString("##vertex-source", vertexSource, 220.0F)) {
            editorState.updateVertexSource(vertexSource);
        }

        std::string fragmentSource = document.fragmentSource;
        ImGui::SeparatorText("Fragment");
        if (editMultilineString("##fragment-source", fragmentSource, 220.0F)) {
            editorState.updateFragmentSource(fragmentSource);
        }
    }
    ImGui::End();
}

void Application::drawRenderViewWindow() {
    if (!showRenderView_) {
        return;
    }

    if (ImGui::Begin("Render View", &showRenderView_)) {
        ImGui::Text("Preview: %s", renderViewPanel_.summary().c_str());
        const std::array<const char*, 5> primitiveIds {"plane", "cube", "torus", "sphere", "cylinder"};
        for (std::size_t index = 0; index < primitiveIds.size(); ++index) {
            if (index > 0) {
                ImGui::SameLine();
            }
            if (ImGui::Button(primitiveIds[index])) {
                renderViewPanel_.choosePrimitive(primitiveIds[index]);
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Reset View")) {
            renderViewPanel_.resetView();
        }
        ImGui::Separator();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const int previewWidth = std::max(1, static_cast<int>(available.x));
        const int previewHeight = std::max(180, static_cast<int>(available.y - ImGui::GetTextLineHeightWithSpacing() * 2.0F));
        const auto& session = renderViewPanel_.renderPreview(previewWidth, previewHeight);
        if (session.previewTextureId != 0) {
            ImGui::Image((ImTextureID)(intptr_t)session.previewTextureId,
                         ImVec2(static_cast<float>(previewWidth), static_cast<float>(previewHeight)),
                         ImVec2(0.0F, 1.0F),
                         ImVec2(1.0F, 0.0F));
            if (ImGui::IsItemHovered()) {
                const ImVec2 dragDelta = ImGui::GetIO().MouseDelta;
                // Left-drag orbits the scene, right-drag pans it inside the preview viewport.
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    renderViewPanel_.orbit(glm::vec2(dragDelta.x * 0.01F, dragDelta.y * 0.01F));
                }
                if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                    renderViewPanel_.pan(glm::vec2(dragDelta.x * 0.005F, dragDelta.y * 0.005F));
                }
            }
        } else {
            ImGui::Dummy(ImVec2(static_cast<float>(previewWidth), static_cast<float>(previewHeight)));
        }
        ImGui::TextUnformatted("Left drag: orbit | Right drag: pan");
        ImGui::TextWrapped("Render status: %s", renderViewPanel_.errorMessage().empty() ? "ok" : renderViewPanel_.errorMessage().c_str());
    }
    ImGui::End();
}

void Application::drawUniformsWindow() {
    if (!showUniforms_) {
        return;
    }

    if (ImGui::Begin("Uniforms", &showUniforms_)) {
        const auto& uniforms = workspace_.uniformState().definitions();
        if (uniforms.empty()) {
            ImGui::TextUnformatted("No uniforms discovered.");
        }

        for (const auto& uniform : uniforms) {
            if (!uniform.editable) {
                ImGui::Text("%s (%s) [read only]", uniform.name.c_str(), uniform.kind.c_str());
                continue;
            }

            if (const auto* value = std::get_if<bool>(&uniform.currentValue)) {
                bool current = *value;
                if (ImGui::Checkbox(uniform.name.c_str(), &current)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<int>(&uniform.currentValue)) {
                int current = *value;
                if (ImGui::InputInt(uniform.name.c_str(), &current)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<float>(&uniform.currentValue)) {
                float current = *value;
                if (ImGui::DragFloat(uniform.name.c_str(), &current, 0.01F)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<glm::vec2>(&uniform.currentValue)) {
                glm::vec2 current = *value;
                if (ImGui::DragFloat2(uniform.name.c_str(), &current.x, 0.01F)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<glm::vec3>(&uniform.currentValue)) {
                glm::vec3 current = *value;
                if (ImGui::DragFloat3(uniform.name.c_str(), &current.x, 0.01F)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<glm::vec4>(&uniform.currentValue)) {
                glm::vec4 current = *value;
                if (ImGui::ColorEdit4(uniform.name.c_str(), &current.x)) {
                    workspace_.applyUniform(uniform.name, current);
                }
                continue;
            }

            if (const auto* value = std::get_if<glm::mat2>(&uniform.currentValue)) {
                glm::mat2 current = *value;
                bool changed = false;
                changed |= ImGui::InputFloat2((uniform.name + "##row0").c_str(), &current[0][0]);
                changed |= ImGui::InputFloat2((uniform.name + "##row1").c_str(), &current[1][0]);
                if (changed) {
                    workspace_.applyUniform(uniform.name, current);
                }
                ImGui::Text("%s (mat2)", uniform.name.c_str());
                continue;
            }

            if (const auto* value = std::get_if<glm::mat3>(&uniform.currentValue)) {
                glm::mat3 current = *value;
                bool changed = false;
                changed |= ImGui::InputFloat3((uniform.name + "##row0").c_str(), &current[0][0]);
                changed |= ImGui::InputFloat3((uniform.name + "##row1").c_str(), &current[1][0]);
                changed |= ImGui::InputFloat3((uniform.name + "##row2").c_str(), &current[2][0]);
                if (changed) {
                    workspace_.applyUniform(uniform.name, current);
                }
                ImGui::Text("%s (mat3)", uniform.name.c_str());
                continue;
            }

            if (const auto* value = std::get_if<glm::mat4>(&uniform.currentValue)) {
                glm::mat4 current = *value;
                bool changed = false;
                changed |= ImGui::InputFloat4((uniform.name + "##row0").c_str(), &current[0][0]);
                changed |= ImGui::InputFloat4((uniform.name + "##row1").c_str(), &current[1][0]);
                changed |= ImGui::InputFloat4((uniform.name + "##row2").c_str(), &current[2][0]);
                changed |= ImGui::InputFloat4((uniform.name + "##row3").c_str(), &current[3][0]);
                if (changed) {
                    workspace_.applyUniform(uniform.name, current);
                }
                ImGui::Text("%s (mat4)", uniform.name.c_str());
                continue;
            }

            ImGui::Text("%s (%s)", uniform.name.c_str(), uniform.kind.c_str());
        }
    }
    ImGui::End();
}

void Application::drawDiagnosticsWindow() {
    if (!showDiagnostics_) {
        return;
    }

    if (ImGui::Begin("Diagnostics", &showDiagnostics_)) {
        ImGui::BeginChild("diagnostics-scroll");
        for (const auto& message : diagnosticsPanel_.messages()) {
            ImGui::TextWrapped("%s", message.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void Application::drawShaderErrorsWindow() {
    if (!showShaderErrors_) {
        return;
    }

    if (ImGui::Begin("Shader Errors", &showShaderErrors_)) {
        ImGui::BeginChild("shader-errors-scroll");
        for (const auto& error : shaderErrorsPanel_.errors()) {
            ImGui::TextWrapped("%s", error.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
}  // namespace shadereditor
