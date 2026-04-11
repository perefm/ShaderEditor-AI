#include "ui/dockspace/DockspaceHost.h"

#include <algorithm>

namespace shadereditor {
void DockspaceHost::registerPanel(const std::string& panelId) {
    if (std::find(panels_.begin(), panels_.end(), panelId) == panels_.end()) {
        panels_.push_back(panelId);
    }
}
}  // namespace shadereditor
