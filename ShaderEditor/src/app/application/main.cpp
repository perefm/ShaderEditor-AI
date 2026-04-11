#include "app/application/Application.h"

int main() {
    shadereditor::Application app;
    if (!app.initialize()) {
        return 1;
    }
    const int code = app.run();
    app.shutdown();
    return code;
}
