#include "app/application/Application.h"

int main() {
    // The application object owns initialization, the UI loop and orderly shutdown.
    shadereditor::Application app;
    if (!app.initialize()) {
        return 1;
    }
    const int code = app.run();
    app.shutdown();
    return code;
}
