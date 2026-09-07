#include "catch2/catch_test_macros.hpp"

#include "app/workspace/WorkspaceLayoutState.h"

TEST_CASE("workspace layout serializes and restores") {
    shadereditor::WorkspaceLayoutState layout;
    layout.setFocusedPanel("render");
    layout.setOpenPanels({"render", "uniforms"});
    const auto serialized = layout.serialize();

    shadereditor::WorkspaceLayoutState restored;
    restored.restore(serialized);
    REQUIRE(restored.focusedPanel() == "render");
    REQUIRE(restored.openPanels().size() == 2);
}
