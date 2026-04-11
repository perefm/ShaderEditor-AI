#pragma once

#include <string>
#include <vector>

namespace shadereditor {
class DockspaceHost {
  public:
    void registerPanel(const std::string& panelId);
    [[nodiscard]] const std::vector<std::string>& panels() const { return panels_; }

  private:
    std::vector<std::string> panels_;
};
}  // namespace shadereditor
