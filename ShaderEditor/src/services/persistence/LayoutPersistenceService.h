#pragma once

#include "app/workspace/WorkspaceLayoutState.h"

#include <filesystem>

namespace shadereditor {
// Persists and restores dock layout state on disk.
class LayoutPersistenceService {
  public:
    void save(const WorkspaceLayoutState& layout, const std::filesystem::path& path) const;
    WorkspaceLayoutState load(const std::filesystem::path& path) const;
};
}  // namespace shadereditor
