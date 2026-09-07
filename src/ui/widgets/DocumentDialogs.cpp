#include "ui/widgets/DocumentDialogs.h"

namespace shadereditor {
std::string DocumentDialogs::unsavedChangesMessage(const ShaderPairDocument& document) const {
    // The UI uses one short message in multiple flows, so keep the wording centralized here.
    return document.isDirty ? "Unsaved shader changes will be discarded." : "No unsaved changes.";
}
}  // namespace shadereditor
