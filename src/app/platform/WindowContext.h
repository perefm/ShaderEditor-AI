#pragma once

#include <string>

struct GLFWwindow;

namespace shadereditor {
// Wraps the GLFW window and the OpenGL context required by the app.
class WindowContext {
  public:
    bool initialize(int width = 1600, int height = 900, const char* title = "Phoenix GLSL shader editor");
    void shutdown();
    bool setVsyncEnabled(bool enabled);
    [[nodiscard]] bool isInitialized() const { return initialized_; }
    [[nodiscard]] const std::string& backend() const { return backend_; }
    [[nodiscard]] GLFWwindow* window() const { return window_; }
    [[nodiscard]] bool vsyncEnabled() const { return vsyncEnabled_; }

  private:
    bool initialized_ {false};
    bool vsyncEnabled_ {true};
    std::string backend_ {"GLFW/OpenGL"};
    GLFWwindow* window_ {nullptr};
};
}  // namespace shadereditor
