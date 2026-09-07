#include "app/workspace/WorkspaceLayoutState.h"

#include <sstream>

namespace shadereditor {
// The layout is stored as a simple newline-delimited file because the amount of state is tiny.
void WorkspaceLayoutState::setOpenPanels(std::vector<std::string> openPanels) { openPanels_ = std::move(openPanels); }

void WorkspaceLayoutState::setFocusedPanel(std::string focusedPanel) { focusedPanel_ = std::move(focusedPanel); }

std::string WorkspaceLayoutState::serialize() const {
    std::ostringstream out;
    out << focusedPanel_;
    for (const auto& panel : openPanels_) {
        out << '\n' << panel;
    }
    return out.str();
}

void WorkspaceLayoutState::restore(const std::string& serialized) {
    std::istringstream input(serialized);
    std::getline(input, focusedPanel_);
    openPanels_.clear();
    std::string line;
    // Every following line represents one visible panel id.
    while (std::getline(input, line)) {
        if (!line.empty()) {
            openPanels_.push_back(line);
        }
    }
}
}  // namespace shadereditor
