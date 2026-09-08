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
std::string formatShaderSource(const std::string& source) {
    std::istringstream input(source);
    std::ostringstream output;
    std::string line;
    int indent = 0;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto first = line.find_first_not_of(" \t");
        const std::string trimmed = first == std::string::npos ? std::string {} : line.substr(first);
        if (trimmed.empty()) {
            continue;
        }
        if (trimmed.rfind("#type ", 0) == 0) {
            indent = 0;
            if (trimmed == "#type fragment" && output.tellp() > std::streampos(0)) {
                output << "\n\n";
            }
            output << trimmed << '\n';
            continue;
        }

        std::string chunk;
        auto emitChunk = [&]() {
            const auto chunkStart = chunk.find_first_not_of(" \t");
            if (chunkStart != std::string::npos) {
                output << std::string(static_cast<std::size_t>(indent) * 4U, ' ')
                       << chunk.substr(chunkStart) << '\n';
            }
            chunk.clear();
        };
        for (const char character : trimmed) {
            if (character == '{') {
                emitChunk();
                output << std::string(static_cast<std::size_t>(indent) * 4U, ' ') << "{" << '\n';
                ++indent;
            } else if (character == '}') {
                emitChunk();
                indent = std::max(0, indent - 1);
                output << std::string(static_cast<std::size_t>(indent) * 4U, ' ') << "}" << '\n';
            } else if (character == ';') {
                chunk += character;
                emitChunk();
            } else {
                chunk += character;
            }
        }
        emitChunk();
    }
    return output.str();
}

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    while (true) {
        const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) {
            break;
        }
        if (copied < buffer.size() - 1) {
            buffer.resize(copied);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
    // Non-Windows builds fall back to the process working directory until a dedicated helper is needed.
    return std::filesystem::current_path();
}

#ifdef _WIN32
std::optional<std::filesystem::path> openFileDialog(
    const wchar_t* title,
    const wchar_t* filter,
    const std::filesystem::path& initialDirectory) {
    std::vector<wchar_t> pathBuffer(32768, L'\0');
    OPENFILENAMEW dialog {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrTitle = title;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = pathBuffer.data();
    dialog.nMaxFile = static_cast<DWORD>(pathBuffer.size());
    const std::wstring initialDirectoryString = initialDirectory.wstring();
    dialog.lpstrInitialDir = initialDirectoryString.c_str();
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&dialog)) {
        return std::nullopt;
    }
    return std::filesystem::path(pathBuffer.data());
}

// "Save As" needs a native save dialog (as opposed to the open dialog above), which lets the
// user type a brand-new file name rather than requiring an existing file to be selected.
std::optional<std::filesystem::path> saveFileDialog(
    const wchar_t* title,
    const wchar_t* filter,
    const wchar_t* defaultExtension,
    const std::filesystem::path& initialDirectory,
    const std::filesystem::path& suggestedFileName) {
    std::vector<wchar_t> pathBuffer(32768, L'\0');
    const std::wstring suggestedFileNameString = suggestedFileName.wstring();
    if (!suggestedFileNameString.empty()) {
        const std::size_t copyLength = std::min(suggestedFileNameString.size(), pathBuffer.size() - 1);
        std::copy_n(suggestedFileNameString.begin(), copyLength, pathBuffer.begin());
    }
    OPENFILENAMEW dialog {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrTitle = title;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = pathBuffer.data();
    dialog.nMaxFile = static_cast<DWORD>(pathBuffer.size());
    const std::wstring initialDirectoryString = initialDirectory.wstring();
    dialog.lpstrInitialDir = initialDirectoryString.c_str();
    dialog.lpstrDefExt = defaultExtension;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&dialog)) {
        return std::nullopt;
    }
    return std::filesystem::path(pathBuffer.data());
}
#endif
}

Application::Application()
    : workspace_(diagnostics_),
      shaderEditorPanel_(workspace_),
      renderViewPanel_(workspace_),
      uniformsPanel_(workspace_),
      diagnosticsPanel_(diagnostics_),
      shaderErrorsPanel_(diagnostics_) {
    shaderEditor_.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
}

bool Application::initialize() {
    const std::string windowTitle = "Phoenix GLSL shader editor v." SHADEREDITOR_VERSION_TIMESTAMP;
    if (!windowContext_.initialize(1600, 900, windowTitle.c_str())) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    imguiIniPath_ = (executableDirectory() / "imgui.ini").string();
    io.IniFilename = imguiIniPath_.c_str();
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#else
    diagnostics_.addError("Installed ImGui package does not expose docking support. Reinstall imgui with the vcpkg feature 'docking-experimental'.");
#endif
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windowContext_.window(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    registerPanels();
    const std::filesystem::path runtimeAssetsDirectory = executableDirectory() / "assets" / "shaders";
    exampleShaderPath_ = runtimeAssetsDirectory / "basic.glsl";
    pixelLightingShaderPath_ = runtimeAssetsDirectory / "pixel_lighting.glsl";
    diagnostics_.addInfo("Runtime assets directory: " + runtimeAssetsDirectory.string());
    loadExampleShaders();
    diagnostics_.addInfo("Application initialized.");
    return true;
}

int Application::run() {
    diagnostics_.addInfo("Using backend: " + windowContext_.backend());
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
    // Keyboard shortcuts are dispatched before panels draw so menu actions and buttons stay in sync.
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
        shaderEditorPanel_.pressCtrlEnter();
    }

    drawWorkspaceHost();
    drawShaderEditorWindow();
    drawRenderViewWindow();
    drawUniformsWindow();
    drawDiagnosticsWindow();
    drawShaderErrorsWindow();
    drawShaderHelpWindow();
}

bool Application::loadExampleShaders() {
    if (!std::filesystem::exists(exampleShaderPath_)) {
        diagnostics_.addError("Example shader is missing from runtime assets folder: " + exampleShaderPath_.parent_path().string());
        return false;
    }

    const bool loaded = workspace_.openShader(exampleShaderPath_);
    if (loaded) {
        diagnostics_.addInfo("Loaded example shader from " + exampleShaderPath_.parent_path().string() + ".");
    }
    return loaded;
}

bool Application::openShaderFromDialog() {
#ifdef _WIN32
    const auto selected = openFileDialog(
        L"Open Shader",
        L"GLSL Shader (*.glsl)\0*.glsl\0All Files (*.*)\0*.*\0",
        executableDirectory());
    if (!selected) {
        return false;
    }

    return workspace_.openShader(*selected);
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

bool Application::saveShaderAsFromDialog() {
#ifdef _WIN32
    // Default to the current document's folder/name so re-saving to a sibling file is a single click.
    const auto& document = workspace_.editorState().document();
    std::filesystem::path initialDirectory = executableDirectory();
    std::filesystem::path suggestedFileName;
    if (document.shaderPath) {
        initialDirectory = document.shaderPath->parent_path();
        suggestedFileName = document.shaderPath->filename();
    }

    const auto selected = saveFileDialog(
        L"Save Shader As",
        L"GLSL Shader (*.glsl)\0*.glsl\0All Files (*.*)\0*.*\0",
        L"glsl",
        initialDirectory,
        suggestedFileName);
    if (!selected) {
        return false;
    }

    return workspace_.saveShadersAs(*selected);
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

bool Application::openModelFromDialog() {
#ifdef _WIN32
    // Filter list mirrors the formats Assimp's default importer registry handles well for
    // Phoenix-style content; "All Files" is kept as a fallback for less common formats.
    const auto selected = openFileDialog(
        L"Open 3D Model",
        L"3D Models (*.glb;*.gltf;*.fbx;*.obj;*.dae)\0*.glb;*.gltf;*.fbx;*.obj;*.dae\0All Files (*.*)\0*.*\0",
        executableDirectory());
    if (!selected) {
        return false;
    }

    return workspace_.openModel(*selected);
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

bool Application::openImageForUniform(const std::string& uniformName) {
#ifdef _WIN32
    const auto selected = openFileDialog(
        L"Open Image",
        L"Images (*.png;*.jpg;*.jpeg;*.bmp;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files (*.*)\0*.*\0",
        executableDirectory());
    if (!selected) {
        return false;
    }
    workspace_.applyUniform(uniformName, selected->string());
    return true;
#else
    diagnostics_.addError("Native file dialogs are only implemented for Windows in this build.");
    return false;
#endif
}

void Application::drawMainMenu() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    // Menu actions mirror the inline buttons from the editor panel.
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open model...")) {
            openModelFromDialog();
        }
        if (ImGui::MenuItem("Open shader...")) {
            openShaderFromDialog();
        }
        if (ImGui::MenuItem("Save shader", "Ctrl+S")) {
            workspace_.saveShaders();
        }
        if (ImGui::MenuItem("Save shader As...")) {
            saveShaderAsFromDialog();
        }
        if (ImGui::MenuItem("Update shader", "Ctrl+Enter")) {
            shaderEditorPanel_.pressUpdateButton();
        }
        if (ImGui::MenuItem("Exit")) {
            glfwSetWindowShouldClose(windowContext_.window(), GLFW_TRUE);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Shader Editor", nullptr, &showShaderEditor_);
        ImGui::MenuItem("Render", nullptr, &showRenderView_);
        ImGui::MenuItem("Uniforms", nullptr, &showUniforms_);
        ImGui::MenuItem("Diagnostics", nullptr, &showDiagnostics_);
        ImGui::MenuItem("Shader Errors", nullptr, &showShaderErrors_);
        ImGui::MenuItem("Shader Help", nullptr, &showShaderHelp_);
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
#ifdef IMGUI_HAS_DOCK
    // The workspace host owns the single dockspace so every major tool panel can be rearranged safely.
    ImGui::DockSpace(ImGui::GetID("WorkspaceDockspace"), ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
#else
    ImGui::TextUnformatted("Docking unavailable in current ImGui build");
#endif
    ImGui::End();
}

void Application::drawShaderEditorWindow() {
    if (!showShaderEditor_) {
        return;
    }

    auto& editorState = workspace_.editorState();
    const auto& document = editorState.document();
    std::string source = document.source;
    if (ImGui::Begin("Shader Editor", &showShaderEditor_)) {
        if (document.isDirty) {
            ImGui::TextUnformatted("(modified)");
        }
        if (ImGui::Button("Open .glsl")) {
            openShaderFromDialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            workspace_.saveShaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save As")) {
            saveShaderAsFromDialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Update Shader")) {
            shaderEditorPanel_.pressUpdateButton();
        }
        ImGui::SameLine();
        if (ImGui::Button("Format Shader")) {
            source = formatShaderSource(source);
            editorState.updateSource(source);
        }
        if (document.isDirty) {
            ImGui::SameLine();
            ImGui::TextWrapped("%s", documentDialogs_.unsavedChangesMessage(document).c_str());
        }

        if (shaderEditorText_ != source) {
            shaderEditor_.SetText(source);
            shaderEditorText_ = source;
        }
        ImVec2 editorSize = ImGui::GetContentRegionAvail();
        editorSize.y = std::max(1.0F, editorSize.y);
        shaderEditor_.Render("##phoenix-source", editorSize, true);
        const std::string editedSource = shaderEditor_.GetText();
        if (editedSource != source) {
            shaderEditorText_ = editedSource;
            editorState.updateSource(editedSource);
        }
    }
    ImGui::End();
}
void Application::drawRenderViewWindow() {
    if (!showRenderView_) {
        return;
    }

    if (ImGui::Begin("Render", &showRenderView_)) {
        const std::array<const char*, 5> primitiveIds {"plane", "cube", "torus", "sphere", "cylinder"};
        for (std::size_t index = 0; index < primitiveIds.size(); ++index) {
            if (index > 0) {
                ImGui::SameLine();
            }
            if (ImGui::Button(primitiveIds[index])) {
                renderViewPanel_.choosePrimitive(primitiveIds[index]);
            }
        }
        // Once a model has been imported (File > Open Model...), let the user switch back and
        // forth between it and the built-in primitives without re-importing (see
        // WorkspaceController::selectModel()).
        if (workspace_.hasLoadedModel()) {
            ImGui::SameLine();
            if (ImGui::Button("model")) {
                workspace_.selectModel();
            }
        }
        // Lets the user import a model directly from this panel, without going through the File
        // menu; reuses the exact same file dialog/import path as File > Open Model...
        ImGui::SameLine();
        if (ImGui::Button("Open model...")) {
            openModelFromDialog();
        }

        // Animation clip picker, shown only when the active model actually has animation clips
        // to choose from (static/untextured meshes and built-in primitives have none).
        const std::vector<std::string> animationNames = renderViewPanel_.animationNames();
        if (!animationNames.empty()) {
            int selectedIndex = renderViewPanel_.selectedAnimationIndex();
            // Slot 0 is always "None" (bind pose) so the user can freeze the model's rest pose.
            const int comboIndex = selectedIndex + 1;
            std::string previewLabel = selectedIndex < 0 ? "None" : animationNames[static_cast<std::size_t>(selectedIndex)];
            ImGui::SetNextItemWidth(220.0F);
            if (ImGui::BeginCombo("Animation", previewLabel.c_str())) {
                const bool noneSelected = comboIndex == 0;
                if (ImGui::Selectable("None", noneSelected)) {
                    renderViewPanel_.selectAnimation(-1);
                }
                for (std::size_t index = 0; index < animationNames.size(); ++index) {
                    const bool isSelected = comboIndex == static_cast<int>(index) + 1;
                    if (ImGui::Selectable(animationNames[index].c_str(), isSelected)) {
                        renderViewPanel_.selectAnimation(static_cast<int>(index));
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Reset View")) {
            renderViewPanel_.resetView();
        }
        ImGui::Separator();

        // Playback transport for Phoenix's time-based auto-uniforms ("t"/"tend"/"beat").
        const PlaybackClockState& playback = workspace_.playbackClock();
        if (playback.isPlaying()) {
            if (ImGui::Button("Pause")) {
                workspace_.pausePreview();
            }
        } else {
            if (ImGui::Button("Play")) {
                workspace_.playPreview();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Time")) {
            workspace_.resetPreview();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(("t = " + std::to_string(playback.elapsedSeconds())).c_str());

        float sectionDuration = playback.sectionDurationSeconds();
        ImGui::SetNextItemWidth(100.0F);
        if (ImGui::InputFloat("tend", &sectionDuration)) {
            workspace_.setSectionDuration(sectionDuration);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(> 1.0 s; resets t when reached)");
        ImGui::SameLine();
        float bpm = playback.bpm();
        ImGui::SetNextItemWidth(100.0F);
        if (ImGui::InputFloat("bpm", &bpm)) {
            workspace_.setBpm(bpm);
        }
        ImGui::Separator();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const int previewWidth = std::max(1, static_cast<int>(available.x));
        const int previewHeight = std::max(180, static_cast<int>(available.y - ImGui::GetTextLineHeightWithSpacing() * 2.0F));
        // The renderer draws to an offscreen OpenGL texture that is then embedded into ImGui.
        const auto& session = renderViewPanel_.renderPreview(previewWidth, previewHeight);
        if (session.previewTextureId != 0) {
            ImGui::Image((ImTextureID)(intptr_t)session.previewTextureId,
                         ImVec2(static_cast<float>(previewWidth), static_cast<float>(previewHeight)),
                         ImVec2(0.0F, 1.0F),
                         ImVec2(1.0F, 0.0F));
            if (ImGui::IsItemHovered()) {
                const ImVec2 dragDelta = ImGui::GetIO().MouseDelta;
                const float wheelDelta = ImGui::GetIO().MouseWheel;
                if (wheelDelta != 0.0F) {
                    renderViewPanel_.zoom(wheelDelta);
                }
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
            // Widget selection is driven by the discovered GLSL type for each uniform.
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

            if (const auto* value = std::get_if<std::string>(&uniform.currentValue)) {
                ImGui::Text("%s (sampler2D)", uniform.name.c_str());
                if (!value->empty()) {
                    ImGui::TextWrapped("%s", value->c_str());
                }
                ImGui::SameLine();
                if (ImGui::Button(("Load Image##" + uniform.name).c_str())) {
                    openImageForUniform(uniform.name);
                }
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
        std::string errors;
        for (const auto& error : shaderErrorsPanel_.errors()) {
            errors += error;
            errors.push_back('\n');
        }
        std::vector<char> buffer(errors.begin(), errors.end());
        buffer.push_back('\0');
        ImGui::InputTextMultiline(
            "##shader-errors-text",
            buffer.data(),
            buffer.size(),
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AllowTabInput);
        ImGui::EndChild();
    }
    ImGui::End();
}

void Application::drawShaderHelpWindow() {
    if (!showShaderHelp_) {
        return;
    }
    if (ImGui::Begin("Shader Help", &showShaderHelp_)) {
        const std::string helpText =
            "Phoenix GLSL globals supplied by ShaderEditor\n\n"
            "The uniforms below are computed and uploaded automatically every frame by the app;\n"
            "they never appear as editable rows in the Uniforms panel, so there is nothing to fill in.\n\n"
            "Camera / transform:\n"
            "  MVP (mat4): model-view-projection matrix for the preview camera.\n"
            "  model (mat4): the preview's model-space rotation matrix (orbit only, no projection);\n"
            "    used by shaders that need to transform normals/tangents into world space.\n"
            "  uCameraPos (vec3): current camera position in world space.\n\n"
            "Playback clock (spec 004 Phoenix auto-uniforms):\n"
            "  t (float): elapsed seconds since playback was last reset; drives time-based effects.\n"
            "  tend (float): configured section duration in seconds (Render panel 'tend' field).\n"
            "  beat (float): normalized phase of the current beat, always in [0, 1); it resets to\n"
            "    0 on each beat boundary and advances according to the configured BPM.\n\n"
            "Imported model material (per active mesh, from the model's own Assimp materials):\n"
            "  Mat_Ka (vec3): ambient color.\n"
            "  Mat_Kd (vec3): diffuse color.\n"
            "  Mat_Ks (vec3): specular color.\n"
            "  Mat_KsStrenght (float): specular strength/shininess.\n"
            "  texture_diffuseN / texture_specularN / texture_normalsN / texture_heightN / ... (sampler2D):\n"
            "    one uniform per texture slot the mesh's material carries, named\n"
            "    \"texture_\" + Phoenix texture type + 1-based index (e.g. texture_diffuse1).\n"
            "    Bound automatically from the imported model's on-disk or embedded (.glb) images.\n\n"
            "Skeletal animation (imported models with bones):\n"
            "  gBones[100] (mat4 array): per-bone skinning matrices for the currently selected\n"
            "    animation clip (Render panel 'Animation' dropdown), recomputed every frame.\n\n"
            "Preview mesh vertex attributes (built-in primitives):\n"
            "  aPos (location 0): vertex position.\n"
            "  aUv (location 1): UV texture coordinate.\n\n"
            "Imported model vertex attributes (Phoenix Mesh::setupMesh layout):\n"
            "  aPos (0), aNormal (1), aTexCoords (2), aTangent (3), aBiTangent (4),\n"
            "  aBoneID (5, uvec4), aBoneWeight (6, vec4).\n\n"
            "sampler2D uniforms you declare yourself: choose an image from the Uniforms panel.\n\n"
            "Declare the inputs in the vertex shader and pass values to the fragment shader using out/in variables.";
        std::vector<char> buffer(helpText.begin(), helpText.end());
        buffer.push_back('\0');
        ImGui::InputTextMultiline(
            "##shader-help-text",
            buffer.data(),
            buffer.size(),
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AllowTabInput);
    }
    ImGui::End();
}
}  // namespace shadereditor
