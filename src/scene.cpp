#include "scene.h"
#include "utils.h"
#include "settings.h"

#include <format>
#include <vector>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>

using namespace comp;
using namespace util;

Scene::Scene(const glm::mat4& mvpinv)
    : MVPInverse{mvpinv}
{
    shaders.insert(std::make_pair("default", Shader{
        {
            {GL_VERTEX_SHADER, "default.vert"},
            {GL_FRAGMENT_SHADER, "default.frag"}
        }
    }));

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert"},
            {GL_FRAGMENT_SHADER, "sdf.frag"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE)
        }
    }));



    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    auto screenSpacedVAO = std::make_shared<VertexArray>(std::vector{
        -1.f, -1.f, 0.0f, // bottom left
        1.f, 1.f, 0.0f,   // top right
        1.f, -1.f, 0.0f,  // bottom right

        -1.f, -1.f, 0.0f, // bottom left
        -1.f, 1.f, 0.0f,   // top left
        1.f, 1.f, 0.0f   // top right
    });
    screenSpacedVAO->vertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    screenMesh = Mesh{screenSpacedVAO, 6};
    glBindVertexArray(0);



    // Setup scene
    for (unsigned int i = 0; i < SCENE_SIZE; ++i) {
        auto entity = EM.create();

        const auto pos = glm::linearRand(glm::vec3{-1.f}, glm::vec3{1.f}) * 0.3f;
        const auto radius = glm::linearRand(0.01f, 0.1f);

        EM.emplace<Sphere>(entity, pos, radius);
        positions.at(i) = glm::vec4{pos, radius};
    }

    sceneBuffer = std::make_unique<VertexArray>(positions);
    sceneBuffer->vertexAttribute(0, 4, GL_FLOAT, GL_FALSE);

    const auto [SCR_WIDTH, SCR_HEIGHT] = get_multiple<0, 1>(Settings::get().to_tuple());

    // Framebuffers:
    auto positionTexture = std::make_shared<Texture<GL_TEXTURE_2D>>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT});
    auto normalTexture = std::make_shared<Texture<GL_TEXTURE_2D>>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT});
    auto depthTexture = std::make_shared<Texture<GL_TEXTURE_2D>>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT}, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT);

    auto sphereFramebuffer = std::make_shared<Framebuffer>(
        std::make_pair(GL_COLOR_ATTACHMENT0, std::shared_ptr{positionTexture}),
        std::make_pair(GL_COLOR_ATTACHMENT1, std::shared_ptr{normalTexture}),
        std::make_pair(GL_DEPTH_ATTACHMENT, std::shared_ptr{depthTexture})
    );
    
    if (!sphereFramebuffer->valid())
        std::cout << "Framebuffer error: " << sphereFramebuffer->completeness() << std::endl;
    assert(sphereFramebuffer->valid());

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void Scene::render() {
    glClear(GL_COLOR_BUFFER_BIT);

    auto runningTime = Settings::get().runningTime;

    if (!shaders.contains("surface"))
        return;

    auto shaderId = *shaders.at("surface");
    glUseProgram(shaderId);
    uniform(shaderId, "MVPInverse", MVPInverse);
    uniform(shaderId, "time", runningTime);
    screenMesh.draw();

    glBindVertexArray(0); // no need to unbind it every time
}