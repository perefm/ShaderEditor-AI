#include "ui/dockspace/DockspaceHost.h"

#include <algorithm>

namespace shadereditor {
void DockspaceHost::registerPanel(const std::string& panelId) {
    // Panel ids are unique so docking state does not accumulate duplicates across restarts.
    if (std::find(panels_.begin(), panels_.end(), panelId) == panels_.end()) {
        panels_.push_back(panelId);
    }
}
}  // namespace shadereditor
