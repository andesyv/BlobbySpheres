#include "scene.h"
#include "utils.h"
#include "settings.h"
#include "camera.h"

#include <format>
#include <vector>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>

using namespace comp;
using namespace util;

Scene::Scene()
{
    shaders.insert(std::make_pair("default", Shader{
        {
            {GL_VERTEX_SHADER, "default.vert.glsl"},
            {GL_FRAGMENT_SHADER, "default.frag.glsl"}
        }
    }));

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE)
        }
    }));

    shaders.insert(std::make_pair("sphere", Shader{
        {
            {GL_VERTEX_SHADER, "sphere.vert.glsl"},
            {GL_GEOMETRY_SHADER, "sphere.geom.glsl"},
            {GL_FRAGMENT_SHADER, "sphere.frag.glsl"}
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
    positionTexture = std::make_shared<Tex2D>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT});
    normalTexture = std::make_shared<Tex2D>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT});
    depthTexture = std::make_shared<Tex2D>(glm::ivec2{SCR_WIDTH, SCR_HEIGHT}, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

    sphereFramebuffer = std::make_shared<Framebuffer>(
        std::make_pair(GL_COLOR_ATTACHMENT0, std::shared_ptr{positionTexture}),
        std::make_pair(GL_COLOR_ATTACHMENT1, std::shared_ptr{normalTexture}),
        std::make_pair(GL_DEPTH_ATTACHMENT, std::shared_ptr{depthTexture})
    );
    
    if (!sphereFramebuffer->valid())
        std::cout << "Framebuffer error: " << sphereFramebuffer->completeness() << std::endl;
    assert(sphereFramebuffer->valid());
}

void Scene::render() {
    const auto runningTime = Settings::get().runningTime;
    const auto& cam = Camera::getGlobalCamera();
    const auto& pMat = cam.getPMat();
    const auto& pMatInverse = cam.getPMatInverse();
    const auto& vMat = cam.getVMat();
    const auto& MVP = cam.getMVP();
    const auto& MVPInverse = cam.getMVPInverse();
    glm::vec4 nearPlane = pMatInverse * glm::vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;
    

    // Sphere pass
    {
        auto g = sphereFramebuffer->guard();
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClearDepth(1.0);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!shaders.contains("sphere"))
            return;
        
        const auto shaderId = *shaders.at("sphere");
        glUseProgram(shaderId);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "modelViewMatrix", vMat);
        uniform(shaderId, "projectionMatrix", pMat);
        uniform(shaderId, "nearPlaneZ", nearPlane.z);
        uniform(shaderId, "time", runningTime);

        auto g2 = sceneBuffer->guard();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(SCENE_SIZE));
    }


    // Surface pass
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!shaders.contains("surface"))
                return;
            
        const auto shaderId = *shaders.at("surface");
        glUseProgram(shaderId);

        positionTexture->bind();
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "time", runningTime);

        screenMesh.draw();
    }

    // glBindVertexArray(0); // no need to unbind it every time
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << std::format("Reloading \"{}\" shader failed!", name) << std::endl;
}