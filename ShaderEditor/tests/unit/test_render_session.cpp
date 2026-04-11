#include "catch2/catch_test_macros.hpp"

#include "editor/ShaderPairDocument.h"
#include "rendering/opengl/PreviewCamera.h"
#include "rendering/opengl/PreviewRenderer.h"
#include "rendering/shaders/UniformIntrospectionService.h"

TEST_CASE("preview renderer updates a valid session") {
    shadereditor::ShaderPairDocument document;
    document.vertexSource = "uniform vec4 u_color; void main(){}";
    document.fragmentSource = "uniform float u_time; void main(){}";
    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);
    shadereditor::PreviewRenderer renderer;
    const auto session = renderer.updatePreview(document, "cube", uniforms);
    REQUIRE(session.errorMessage.empty());
    REQUIRE(session.selectedPrimitiveId == "cube");
}

TEST_CASE("preview camera produces a stable transform matrix") {
    shadereditor::PreviewInteractionState interaction;
    interaction.orbit(glm::vec2(0.2F, 0.1F));
    interaction.pan(glm::vec2(0.1F, 0.0F));

    shadereditor::PreviewCamera camera;
    const glm::mat4 viewProjection = camera.viewProjection(interaction, 1.0F);

    REQUIRE(viewProjection[0][0] != 0.0F);
}

TEST_CASE("preview camera pan changes the view projection matrix") {
    shadereditor::PreviewCamera camera;
    shadereditor::PreviewInteractionState baseInteraction;
    shadereditor::PreviewInteractionState pannedInteraction;
    pannedInteraction.pan(glm::vec2(0.5F, 0.25F));

    const glm::mat4 baseMatrix = camera.viewProjection(baseInteraction, 1.0F);
    const glm::mat4 pannedMatrix = camera.viewProjection(pannedInteraction, 1.0F);

    REQUIRE(baseMatrix[3][0] != pannedMatrix[3][0]);
}
