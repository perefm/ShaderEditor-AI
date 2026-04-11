#pragma once

#include <string>
#include <vector>

namespace shadereditor {
// Collects informational and shader-specific messages shown in the diagnostics UI.
class DiagnosticsState {
  public:
    void addInfo(std::string message);
    void addError(std::string message);
    void clear();
    [[nodiscard]] const std::vector<std::string>& messages() const { return messages_; }
    [[nodiscard]] const std::vector<std::string>& shaderErrors() const { return shaderErrors_; }

  private:
    std::vector<std::string> messages_;
    std::vector<std::string> shaderErrors_;
};
}  // namespace shadereditor
