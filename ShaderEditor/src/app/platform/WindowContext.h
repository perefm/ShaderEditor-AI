#pragma once

#include <string>

struct GLFWwindow;

namespace shadereditor {
class WindowContext {
  public:
    bool initialize(int width = 1600, int height = 900, const char* title = "Shader Editor");
    void shutdown();
    [[nodiscard]] bool isInitialized() const { return initialized_; }
    [[nodiscard]] const std::string& backend() const { return backend_; }
    [[nodiscard]] GLFWwindow* window() const { return window_; }

  private:
    bool initialized_ {false};
    std::string backend_ {"GLFW/OpenGL"};
    GLFWwindow* window_ {nullptr};
};
}  // namespace shadereditor
