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
    // One UV per vertex, generated per-shape (see BuiltInPrimitives.cpp) so each primitive gets a
    // mapping appropriate to its own topology (e.g. the cube gets one full copy of the texture per
    // face, like a die, rather than a single spherical projection shared by every shape).
    std::vector<glm::vec2> texcoords;
    // Per-vertex normals/tangents/biTangents mirror the Assimp model vertex
    // layout (see ModelVertex) so that any shader designed for imported
    // models can also render built-in primitives without a layout mismatch.
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> biTangents;
    bool available {true};
};
}  // namespace shadereditor
