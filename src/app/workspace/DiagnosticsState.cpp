#include "app/workspace/DiagnosticsState.h"

namespace shadereditor {
namespace {
// Prefix diagnostic entries so the plain-text diagnostics panel can display their severity.
std::string formatMessage(const char* level, const std::string& message) {
    return "[" + std::string(level) + "] " + message;
}
}

void DiagnosticsState::addInfo(std::string message) { messages_.push_back(formatMessage("info", message)); }

void DiagnosticsState::addError(std::string message) {
    // Shader errors are duplicated into the general log so both panels stay in sync.
    const auto formatted = formatMessage("error", message);
    shaderErrors_.push_back(formatted);
    messages_.push_back(formatted);
}

void DiagnosticsState::clearShaderErrors() { shaderErrors_.clear(); }

void DiagnosticsState::clear() {
    messages_.clear();
    shaderErrors_.clear();
}
}  // namespace shadereditor
