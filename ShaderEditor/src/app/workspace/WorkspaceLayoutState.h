#pragma once

#include <string>
#include <vector>

namespace shadereditor {
// Serializable description of which docked panels are visible and focused.
class WorkspaceLayoutState {
  public:
    void setOpenPanels(std::vector<std::string> openPanels);
    void setFocusedPanel(std::string focusedPanel);
    [[nodiscard]] const std::vector<std::string>& openPanels() const { return openPanels_; }
    [[nodiscard]] const std::string& focusedPanel() const { return focusedPanel_; }
    [[nodiscard]] std::string serialize() const;
    void restore(const std::string& serialized);

  private:
    std::vector<std::string> openPanels_;
    std::string focusedPanel_;
};
}  // namespace shadereditor
