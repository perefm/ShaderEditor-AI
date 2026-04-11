#include "app/platform/WindowContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace shadereditor {
bool WindowContext::initialize(int width, int height, const char* title) {
    // GLFW owns both the native window and the OpenGL context lifetime for the app.
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    // Keep preview interaction responsive while avoiding busy-loop rendering.
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    initialized_ = true;
    return true;
}

void WindowContext::shutdown() {
    // Destroy the context before terminating GLFW so backend shutdown stays ordered.
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
    initialized_ = false;
}
}  // namespace shadereditor
