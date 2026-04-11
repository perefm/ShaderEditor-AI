#include "app/workspace/DiagnosticsState.h"

namespace shadereditor {
namespace {
std::string formatMessage(const char* level, const std::string& message) {
    return "[" + std::string(level) + "] " + message;
}
}

void DiagnosticsState::addInfo(std::string message) { messages_.push_back(formatMessage("info", message)); }

void DiagnosticsState::addError(std::string message) {
    const auto formatted = formatMessage("error", message);
    shaderErrors_.push_back(formatted);
    messages_.push_back(formatted);
}

void DiagnosticsState::clear() {
    messages_.clear();
    shaderErrors_.clear();
}
}  // namespace shadereditor
