#include "rendering/geometry/BuiltInPrimitives.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <algorithm>
#include <cstddef>

namespace shadereditor {
namespace {
constexpr float kPi = 3.14159265359F;

void appendTriangle(
    std::vector<glm::vec3>& vertices, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    vertices.insert(vertices.end(), {a, b, c});
}

void appendTriangleUv(
    std::vector<glm::vec2>& texcoords, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
    texcoords.insert(texcoords.end(), {a, b, c});
}

// Appends a quad (as two triangles) together with its own explicit UV rectangle, so callers can
// map each face of a shape independently instead of deriving UVs from vertex position afterwards.
void appendQuad(
    PreviewPrimitive& primitive,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
    const glm::vec2& uvA, const glm::vec2& uvB, const glm::vec2& uvC, const glm::vec2& uvD) {
    appendTriangle(primitive.vertices, a, b, c);
    appendTriangle(primitive.vertices, a, c, d);
    appendTriangleUv(primitive.texcoords, uvA, uvB, uvC);
    appendTriangleUv(primitive.texcoords, uvA, uvC, uvD);
}

void appendTriangleWithUv(
    PreviewPrimitive& primitive,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
    const glm::vec2& uvA, const glm::vec2& uvB, const glm::vec2& uvC) {
    appendTriangle(primitive.vertices, a, b, c);
    appendTriangleUv(primitive.texcoords, uvA, uvB, uvC);
}

// Built-in primitives are unindexed triangle soups (every vertex is
// duplicated per triangle), so a flat per-face normal/tangent is enough to
// give them a layout compatible with Assimp-model shaders (mega/toon/rim
// material shaders), which expect a real normal/tangent/biTangent attribute.
void generateFaceNormalsAndTangents(PreviewPrimitive& primitive) {
    const std::size_t vertexCount = primitive.vertices.size();
    primitive.normals.assign(vertexCount, glm::vec3 {0.0F, 0.0F, 1.0F});
    primitive.tangents.assign(vertexCount, glm::vec3 {1.0F, 0.0F, 0.0F});
    primitive.biTangents.assign(vertexCount, glm::vec3 {0.0F, 1.0F, 0.0F});

    for (std::size_t i = 0; i + 2 < vertexCount; i += 3) {
        const glm::vec3& p0 = primitive.vertices[i];
        const glm::vec3& p1 = primitive.vertices[i + 1];
        const glm::vec3& p2 = primitive.vertices[i + 2];
        const glm::vec3 edge1 = p1 - p0;
        const glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::cross(edge1, edge2);
        const float normalLength = glm::length(normal);
        normal = normalLength > 1e-8F ? normal / normalLength : glm::vec3 {0.0F, 0.0F, 1.0F};

        glm::vec3 tangent {1.0F, 0.0F, 0.0F};
        if (i + 2 < primitive.texcoords.size()) {
            const glm::vec2& uv0 = primitive.texcoords[i];
            const glm::vec2& uv1 = primitive.texcoords[i + 1];
            const glm::vec2& uv2 = primitive.texcoords[i + 2];
            const glm::vec2 deltaUv1 = uv1 - uv0;
            const glm::vec2 deltaUv2 = uv2 - uv0;
            const float det = deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y;
            if (std::fabs(det) > 1e-8F) {
                const float invDet = 1.0F / det;
                tangent = invDet * (edge1 * deltaUv2.y - edge2 * deltaUv1.y);
                const float tangentLength = glm::length(tangent);
                if (tangentLength > 1e-8F) {
                    tangent /= tangentLength;
                }
            }
        }
        // Re-orthogonalize against the normal (Gram-Schmidt) so tangent/normal stay perpendicular.
        tangent = tangent - normal * glm::dot(normal, tangent);
        const float tangentLength = glm::length(tangent);
        tangent = tangentLength > 1e-8F ? tangent / tangentLength : glm::vec3 {1.0F, 0.0F, 0.0F};
        const glm::vec3 biTangent = glm::cross(normal, tangent);

        for (std::size_t v = i; v < i + 3; ++v) {
            primitive.normals[v] = normal;
            primitive.tangents[v] = tangent;
            primitive.biTangents[v] = biTangent;
        }
    }
}

PreviewPrimitive makePlane() {
    PreviewPrimitive primitive {"plane", "Plane", {}};
    appendQuad(
        primitive,
        {-1.0F, -1.0F, 0.0F}, {1.0F, -1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}, {-1.0F, 1.0F, 0.0F},
        {0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F});
    return primitive;
}

// Maps the whole [0,1]x[0,1] texture onto every face independently ("dice" mapping), so a texture
// (e.g. a numbered die face, a wood crate texture, a checker pattern) reads correctly and
// undistorted on each of the cube's six faces, rather than being derived from a spherical
// projection of the vertex positions (which is what previously made every face show only a sliver
// of the texture, wrapped around the cube like a sphere).
PreviewPrimitive makeCube() {
    PreviewPrimitive primitive {"cube", "Cube", {}};
    constexpr glm::vec2 uv00 {0.0F, 0.0F};
    constexpr glm::vec2 uv10 {1.0F, 0.0F};
    constexpr glm::vec2 uv11 {1.0F, 1.0F};
    constexpr glm::vec2 uv01 {0.0F, 1.0F};
    // Front (+Z)
    appendQuad(primitive, {-1.0F, -1.0F, 1.0F}, {1.0F, -1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {-1.0F, 1.0F, 1.0F}, uv00, uv10, uv11, uv01);
    // Back (-Z)
    appendQuad(primitive, {1.0F, -1.0F, -1.0F}, {-1.0F, -1.0F, -1.0F}, {-1.0F, 1.0F, -1.0F}, {1.0F, 1.0F, -1.0F}, uv00, uv10, uv11, uv01);
    // Left (-X)
    appendQuad(primitive, {-1.0F, -1.0F, -1.0F}, {-1.0F, -1.0F, 1.0F}, {-1.0F, 1.0F, 1.0F}, {-1.0F, 1.0F, -1.0F}, uv00, uv10, uv11, uv01);
    // Right (+X)
    appendQuad(primitive, {1.0F, -1.0F, 1.0F}, {1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, uv00, uv10, uv11, uv01);
    // Top (+Y)
    appendQuad(primitive, {-1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, -1.0F}, {-1.0F, 1.0F, -1.0F}, uv00, uv10, uv11, uv01);
    // Bottom (-Y)
    appendQuad(primitive, {-1.0F, -1.0F, -1.0F}, {1.0F, -1.0F, -1.0F}, {1.0F, -1.0F, 1.0F}, {-1.0F, -1.0F, 1.0F}, uv00, uv10, uv11, uv01);
    return primitive;
}

// Standard equirectangular (latitude/longitude) mapping: u sweeps around the sphere with the
// slice/theta angle, v sweeps from pole to pole with the stack/phi angle - the conventional
// mapping for spheres, distinct from the cube's per-face mapping above.
PreviewPrimitive makeSphere() {
    PreviewPrimitive primitive {"sphere", "Sphere", {}};
    constexpr int stacks = 12;
    constexpr int slices = 16;
    for (int stack = 0; stack < stacks; ++stack) {
        const float phi0 = kPi * (static_cast<float>(stack) / stacks - 0.5F);
        const float phi1 = kPi * (static_cast<float>(stack + 1) / stacks - 0.5F);
        const float v0 = static_cast<float>(stack) / stacks;
        const float v1 = static_cast<float>(stack + 1) / stacks;
        for (int slice = 0; slice < slices; ++slice) {
            const float theta0 = 2.0F * kPi * static_cast<float>(slice) / slices;
            const float theta1 = 2.0F * kPi * static_cast<float>(slice + 1) / slices;
            const float u0 = static_cast<float>(slice) / slices;
            const float u1 = static_cast<float>(slice + 1) / slices;
            const glm::vec3 a {std::cos(phi0) * std::cos(theta0), std::sin(phi0), std::cos(phi0) * std::sin(theta0)};
            const glm::vec3 b {std::cos(phi0) * std::cos(theta1), std::sin(phi0), std::cos(phi0) * std::sin(theta1)};
            const glm::vec3 c {std::cos(phi1) * std::cos(theta1), std::sin(phi1), std::cos(phi1) * std::sin(theta1)};
            const glm::vec3 d {std::cos(phi1) * std::cos(theta0), std::sin(phi1), std::cos(phi1) * std::sin(theta0)};
            appendQuad(primitive, a, b, c, d, {u0, v0}, {u1, v0}, {u1, v1}, {u0, v1});
        }
    }
    return primitive;
}

// The side wraps the texture once around the circumference (u) and once top-to-bottom (v); each
// cap gets its own disc-shaped mapping centered at (0.5, 0.5) so the caps read as a circular label
// rather than reusing the side's cylindrical UVs.
PreviewPrimitive makeCylinder() {
    PreviewPrimitive primitive {"cylinder", "Cylinder", {}};
    constexpr int slices = 24;
    for (int slice = 0; slice < slices; ++slice) {
        const float theta0 = 2.0F * kPi * static_cast<float>(slice) / slices;
        const float theta1 = 2.0F * kPi * static_cast<float>(slice + 1) / slices;
        const float u0 = static_cast<float>(slice) / slices;
        const float u1 = static_cast<float>(slice + 1) / slices;
        const glm::vec3 b0 {std::cos(theta0), -1.0F, std::sin(theta0)};
        const glm::vec3 b1 {std::cos(theta1), -1.0F, std::sin(theta1)};
        const glm::vec3 t1 {std::cos(theta1), 1.0F, std::sin(theta1)};
        const glm::vec3 t0 {std::cos(theta0), 1.0F, std::sin(theta0)};
        appendQuad(primitive, b0, b1, t1, t0, {u0, 0.0F}, {u1, 0.0F}, {u1, 1.0F}, {u0, 1.0F});

        const glm::vec2 capCenter {0.5F, 0.5F};
        const glm::vec2 capUv0 {0.5F + 0.5F * std::cos(theta0), 0.5F + 0.5F * std::sin(theta0)};
        const glm::vec2 capUv1 {0.5F + 0.5F * std::cos(theta1), 0.5F + 0.5F * std::sin(theta1)};
        appendTriangleWithUv(primitive, {0.0F, 1.0F, 0.0F}, t0, t1, capCenter, capUv0, capUv1);
        appendTriangleWithUv(primitive, {0.0F, -1.0F, 0.0F}, b1, b0, capCenter, capUv1, capUv0);
    }
    return primitive;
}

// Toroidal mapping: u follows the major angle (around the ring), v follows the minor angle
// (around the tube's cross-section), so the texture wraps smoothly around both loops of the torus.
PreviewPrimitive makeTorus() {
    PreviewPrimitive primitive {"torus", "Torus", {}};
    constexpr int majorSegments = 24;
    constexpr int minorSegments = 12;
    constexpr float majorRadius = 1.1F;
    constexpr float minorRadius = 0.35F;
    for (int major = 0; major < majorSegments; ++major) {
        const float phi0 = 2.0F * kPi * static_cast<float>(major) / majorSegments;
        const float phi1 = 2.0F * kPi * static_cast<float>(major + 1) / majorSegments;
        const float u0 = static_cast<float>(major) / majorSegments;
        const float u1 = static_cast<float>(major + 1) / majorSegments;
        for (int minor = 0; minor < minorSegments; ++minor) {
            const float theta0 = 2.0F * kPi * static_cast<float>(minor) / minorSegments;
            const float theta1 = 2.0F * kPi * static_cast<float>(minor + 1) / minorSegments;
            const float v0 = static_cast<float>(minor) / minorSegments;
            const float v1 = static_cast<float>(minor + 1) / minorSegments;
            auto point = [](float phi, float theta) {
                const float r = majorRadius + minorRadius * std::cos(theta);
                return glm::vec3 {r * std::cos(phi), minorRadius * std::sin(theta), r * std::sin(phi)};
            };
            appendQuad(
                primitive,
                point(phi0, theta0), point(phi1, theta0), point(phi1, theta1), point(phi0, theta1),
                {u0, v0}, {u1, v0}, {u1, v1}, {u0, v1});
        }
    }
    return primitive;
}
}

std::vector<PreviewPrimitive> makeBuiltInPrimitives() {
    auto primitives = std::vector<PreviewPrimitive> {
        makePlane(),
        makeCube(),
        makeTorus(),
        makeSphere(),
        makeCylinder(),
    };
    // Textures are loaded with stbi_set_flip_vertically_on_load(1) (see
    // PreviewRenderer::textureForPath/applyUniforms), which stores row 0 of the resulting GL
    // texture as the image's bottom row. Every shape above was authored with the conventional
    // "v=0 at the top of the texture" convention, so without this flip every primitive displayed
    // its texture upside-down. Flipping v once, in a single place after all shapes are generated,
    // keeps each makeXxx() function's mapping easy to reason about on its own while still
    // producing a right-side-up texture for every primitive.
    for (auto& primitive : primitives) {
        for (auto& uv : primitive.texcoords) {
            uv.y = 1.0F - uv.y;
        }
        generateFaceNormalsAndTangents(primitive);
    }
    return primitives;
}
}  // namespace shadereditor
