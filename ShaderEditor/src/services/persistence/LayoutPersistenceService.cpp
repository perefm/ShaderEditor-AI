#include "services/persistence/LayoutPersistenceService.h"

#include <fstream>
#include <sstream>

namespace shadereditor {
void LayoutPersistenceService::save(const WorkspaceLayoutState& layout, const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << layout.serialize();
}

WorkspaceLayoutState LayoutPersistenceService::load(const std::filesystem::path& path) const {
    WorkspaceLayoutState layout;
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    layout.restore(buffer.str());
    return layout;
}
}  // namespace shadereditor
