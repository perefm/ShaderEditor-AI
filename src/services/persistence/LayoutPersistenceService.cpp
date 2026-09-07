#include "services/persistence/LayoutPersistenceService.h"

#include <fstream>
#include <sstream>

namespace shadereditor {
void LayoutPersistenceService::save(const WorkspaceLayoutState& layout, const std::filesystem::path& path) const {
    // The layout payload is already serialized by WorkspaceLayoutState.
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << layout.serialize();
}

WorkspaceLayoutState LayoutPersistenceService::load(const std::filesystem::path& path) const {
    WorkspaceLayoutState layout;
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    // The persistence layer stays intentionally thin and delegates parsing back to the model.
    buffer << input.rdbuf();
    layout.restore(buffer.str());
    return layout;
}
}  // namespace shadereditor
