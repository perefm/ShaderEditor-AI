#include "app/platform/WindowContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#endif

namespace shadereditor {
bool WindowContext::initialize(int width, int height, const char* title) {
    // GLFW owns both the native window and the OpenGL context lifetime for the app.
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        return false;
    }

#ifdef _WIN32
    if (const auto icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101)); icon != nullptr) {
        const auto nativeWindow = glfwGetWin32Window(window_);
        SendMessageW(nativeWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        SendMessageW(nativeWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    }
#endif

    glfwMakeContextCurrent(window_);
    // Keep preview interaction responsive while avoiding busy-loop rendering.
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    if (GLVersion.major < 4 || (GLVersion.major == 4 && GLVersion.minor < 6)) {
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
