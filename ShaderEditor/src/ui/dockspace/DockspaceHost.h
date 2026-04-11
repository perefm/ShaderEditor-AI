#pragma once

#include <string>
#include <vector>

namespace shadereditor {
// Tracks the panel ids that are eligible to live in the shared dockspace.
class DockspaceHost {
  public:
    void registerPanel(const std::string& panelId);
    [[nodiscard]] const std::vector<std::string>& panels() const { return panels_; }

  private:
    std::vector<std::string> panels_;
};
}  // namespace shadereditor
