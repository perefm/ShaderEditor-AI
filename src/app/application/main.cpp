#include "app/application/Application.h"

namespace {
int runApplication() {
    // The application object owns initialization, the UI loop and orderly shutdown.
    shadereditor::Application app;
    if (!app.initialize()) {
        return 1;
    }
    const int code = app.run();
    app.shutdown();
    return code;
}
}  // namespace

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return runApplication(); }
#else
int main() { return runApplication(); }
#endif
