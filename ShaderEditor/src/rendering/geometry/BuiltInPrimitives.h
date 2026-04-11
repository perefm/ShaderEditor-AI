#pragma once

#include "rendering/geometry/PreviewPrimitive.h"

#include <vector>

namespace shadereditor {
// Creates the built-in preview meshes used by the primitive library.
std::vector<PreviewPrimitive> makeBuiltInPrimitives();
}
