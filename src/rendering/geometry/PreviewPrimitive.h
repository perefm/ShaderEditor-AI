#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <string>
#include <vector>

namespace shadereditor {
// CPU-side description of a preview mesh before it is uploaded to OpenGL.
struct PreviewPrimitive {
    std::string id;
    std::string label;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    bool available {true};
};
}  // namespace shadereditor
