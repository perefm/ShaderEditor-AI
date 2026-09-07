#include "rendering/geometry/BuiltInPrimitives.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <cmath>
#include <algorithm>

namespace shadereditor {
namespace {
constexpr float kPi = 3.14159265359F;

void generateUvMap(PreviewPrimitive& primitive) {
    primitive.texcoords.reserve(primitive.vertices.size());
    for (const auto& position : primitive.vertices) {
        const float u = primitive.id == "plane"
            ? (position.x + 1.0F) * 0.5F
            : 0.5F + std::atan2(position.z, position.x) / (2.0F * kPi);
        const float v = primitive.id == "plane"
            ? (position.y + 1.0F) * 0.5F
            : 0.5F + std::asin(std::clamp(position.y, -1.0F, 1.0F)) / kPi;
        primitive.texcoords.emplace_back(u, v);
    }
}

void appendTriangle(std::vector<glm::vec3>& vertices, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    vertices.insert(vertices.end(), {a, b, c});
}

void appendQuad(std::vector<glm::vec3>& vertices, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d) {
    appendTriangle(vertices, a, b, c);
    appendTriangle(vertices, a, c, d);
}

PreviewPrimitive makePlane() {
    PreviewPrimitive primitive {"plane", "Plane", {}};
    appendQuad(primitive.vertices, {-1.0F, -1.0F, 0.0F}, {1.0F, -1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}, {-1.0F, 1.0F, 0.0F});
    return primitive;
}

PreviewPrimitive makeCube() {
    PreviewPrimitive primitive {"cube", "Cube", {}};
    appendQuad(primitive.vertices, {-1.0F, -1.0F, 1.0F}, {1.0F, -1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {-1.0F, 1.0F, 1.0F});
    appendQuad(primitive.vertices, {1.0F, -1.0F, -1.0F}, {-1.0F, -1.0F, -1.0F}, {-1.0F, 1.0F, -1.0F}, {1.0F, 1.0F, -1.0F});
    appendQuad(primitive.vertices, {-1.0F, -1.0F, -1.0F}, {-1.0F, -1.0F, 1.0F}, {-1.0F, 1.0F, 1.0F}, {-1.0F, 1.0F, -1.0F});
    appendQuad(primitive.vertices, {1.0F, -1.0F, 1.0F}, {1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, -1.0F}, {1.0F, 1.0F, 1.0F});
    appendQuad(primitive.vertices, {-1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, -1.0F}, {-1.0F, 1.0F, -1.0F});
    appendQuad(primitive.vertices, {-1.0F, -1.0F, -1.0F}, {1.0F, -1.0F, -1.0F}, {1.0F, -1.0F, 1.0F}, {-1.0F, -1.0F, 1.0F});
    return primitive;
}

PreviewPrimitive makeSphere() {
    PreviewPrimitive primitive {"sphere", "Sphere", {}};
    constexpr int stacks = 12;
    constexpr int slices = 16;
    for (int stack = 0; stack < stacks; ++stack) {
        const float phi0 = kPi * (static_cast<float>(stack) / stacks - 0.5F);
        const float phi1 = kPi * (static_cast<float>(stack + 1) / stacks - 0.5F);
        for (int slice = 0; slice < slices; ++slice) {
            const float theta0 = 2.0F * kPi * static_cast<float>(slice) / slices;
            const float theta1 = 2.0F * kPi * static_cast<float>(slice + 1) / slices;
            const glm::vec3 a {std::cos(phi0) * std::cos(theta0), std::sin(phi0), std::cos(phi0) * std::sin(theta0)};
            const glm::vec3 b {std::cos(phi0) * std::cos(theta1), std::sin(phi0), std::cos(phi0) * std::sin(theta1)};
            const glm::vec3 c {std::cos(phi1) * std::cos(theta1), std::sin(phi1), std::cos(phi1) * std::sin(theta1)};
            const glm::vec3 d {std::cos(phi1) * std::cos(theta0), std::sin(phi1), std::cos(phi1) * std::sin(theta0)};
            appendQuad(primitive.vertices, a, b, c, d);
        }
    }
    return primitive;
}

PreviewPrimitive makeCylinder() {
    PreviewPrimitive primitive {"cylinder", "Cylinder", {}};
    constexpr int slices = 24;
    for (int slice = 0; slice < slices; ++slice) {
        const float theta0 = 2.0F * kPi * static_cast<float>(slice) / slices;
        const float theta1 = 2.0F * kPi * static_cast<float>(slice + 1) / slices;
        const glm::vec3 b0 {std::cos(theta0), -1.0F, std::sin(theta0)};
        const glm::vec3 b1 {std::cos(theta1), -1.0F, std::sin(theta1)};
        const glm::vec3 t1 {std::cos(theta1), 1.0F, std::sin(theta1)};
        const glm::vec3 t0 {std::cos(theta0), 1.0F, std::sin(theta0)};
        appendQuad(primitive.vertices, b0, b1, t1, t0);
        appendTriangle(primitive.vertices, {0.0F, 1.0F, 0.0F}, t0, t1);
        appendTriangle(primitive.vertices, {0.0F, -1.0F, 0.0F}, b1, b0);
    }
    return primitive;
}

PreviewPrimitive makeTorus() {
    PreviewPrimitive primitive {"torus", "Torus", {}};
    constexpr int majorSegments = 24;
    constexpr int minorSegments = 12;
    constexpr float majorRadius = 1.1F;
    constexpr float minorRadius = 0.35F;
    for (int major = 0; major < majorSegments; ++major) {
        const float phi0 = 2.0F * kPi * static_cast<float>(major) / majorSegments;
        const float phi1 = 2.0F * kPi * static_cast<float>(major + 1) / majorSegments;
        for (int minor = 0; minor < minorSegments; ++minor) {
            const float theta0 = 2.0F * kPi * static_cast<float>(minor) / minorSegments;
            const float theta1 = 2.0F * kPi * static_cast<float>(minor + 1) / minorSegments;
            auto point = [](float phi, float theta) {
                const float r = majorRadius + minorRadius * std::cos(theta);
                return glm::vec3 {r * std::cos(phi), minorRadius * std::sin(theta), r * std::sin(phi)};
            };
            appendQuad(primitive.vertices, point(phi0, theta0), point(phi1, theta0), point(phi1, theta1), point(phi0, theta1));
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
    for (auto& primitive : primitives) {
        generateUvMap(primitive);
    }
    return primitives;
}
}  // namespace shadereditor
