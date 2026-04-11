#pragma once

#include <glm/vec3.hpp>

#include <string>
#include <vector>

namespace shadereditor {
struct PreviewPrimitive {
    std::string id;
    std::string label;
    std::vector<glm::vec3> vertices;
    bool available {true};
};
}  // namespace shadereditor
