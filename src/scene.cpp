#include "scene.h"
#include "utils.h"
#include "settings.h"
#include "camera.h"

#include <format>
#include <vector>
#include <iostream>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <imgui.h>

using namespace comp;
using namespace util;

constexpr std::size_t SCENE_SIZE = 1000u;
constexpr auto LIST_MAX_ENTRIES = 14u;

Scene::Scene()
{
    const auto SCR_SIZE = Settings::get().SCR_SIZE;

    shaders.insert(std::make_pair("default", Shader{
        {
            {GL_VERTEX_SHADER, "default.vert.glsl"},
            {GL_FRAGMENT_SHADER, "default.frag.glsl"}
        }
    }));

    shaders.insert(std::make_pair("sphere", Shader{
        {
            {GL_VERTEX_SHADER, "sphere.vert.glsl"},
            {GL_GEOMETRY_SHADER, "sphere.geom.glsl"},
            {GL_FRAGMENT_SHADER, "sphere.frag.glsl"}
        }
    }));

    shaders.insert(std::make_pair("list", Shader{
        {
            {GL_VERTEX_SHADER, "sphere.vert.glsl"},
            {GL_GEOMETRY_SHADER, "sphere.geom.glsl"},
            {GL_FRAGMENT_SHADER, "list.frag.glsl"}
        }, {
            std::format("MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
            std::format("SCREEN_SIZE uvec2({},{})", SCR_SIZE.x, SCR_SIZE.y)
        }
    }));

    int param{0};
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &param);
    assert(2 + LIST_MAX_ENTRIES <= param);

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "pixelquads.vert.glsl"},
            {GL_GEOMETRY_SHADER, "pixelquad.geom.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE),
            std::format("MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
            std::format("SCREEN_SIZE uvec2({},{})", SCR_SIZE.x, SCR_SIZE.y)
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
    std::array<glm::vec4, SCENE_SIZE> positions;

    for (unsigned int i = 0; i < SCENE_SIZE; ++i) {
        auto entity = EM.create();

        const auto pos = glm::ballRand(0.5f);
        const auto radius = glm::linearRand(0.01f, 0.1f);

        EM.emplace<Sphere>(entity, pos, radius);
        positions.at(i) = glm::vec4{pos, radius};
    }

    sceneBuffer = std::make_unique<VertexArray>(positions);
    sceneBuffer->vertexAttribute(0, 4, GL_FLOAT, GL_FALSE);

    // Framebuffers:
    positionTexture = std::make_shared<Tex2D>(SCR_SIZE);
    normalTexture = std::make_shared<Tex2D>(SCR_SIZE);
    depthTexture = std::make_shared<Tex2D>(SCR_SIZE, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

    sphereFramebuffer = std::make_shared<Framebuffer>(
        std::make_pair(GL_COLOR_ATTACHMENT0, std::shared_ptr{positionTexture}),
        std::make_pair(GL_COLOR_ATTACHMENT1, std::shared_ptr{normalTexture}),
        std::make_pair(GL_DEPTH_ATTACHMENT, std::shared_ptr{depthTexture})
    );
    
    assert(sphereFramebuffer->completeness() == "GL_FRAMEBUFFER_COMPLETE");


    // SSBOs
    const std::size_t bufferSize = (sizeof(glm::vec4) * LIST_MAX_ENTRIES + sizeof(glm::uvec4)) * SCR_SIZE.x * SCR_SIZE.y;
    listBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(bufferSize, GL_DYNAMIC_DRAW);

    // listPixelBuffer = std::make_unique<VertexArray>();
    // auto g = listBuffer->guard();
    // listPixelBuffer->vertexAttribute(0, 1, GL_UNSIGNED_INT, GL_FALSE); // count
    // listPixelBuffer->vertexAttribute(1, 2, GL_UNSIGNED_INT, GL_FALSE); // screenCoord
    // for (GLuint i{0}; i < LIST_MAX_ENTRIES; ++i) // pos[MAX_ENTRIES]
    //     listPixelBuffer->vertexAttribute(2 + i, 4, GL_FLOAT, GL_FALSE);
    
    glBindVertexArray(0);
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << std::format("Reloading \"{}\" shader failed!", name) << std::endl;
}

void Scene::render() {
    const auto runningTime = Settings::get().runningTime;
    const auto SCR_SIZE = Settings::get().SCR_SIZE;
    const auto& cam = Camera::getGlobalCamera();
    const auto& pMat = cam.getPMat();
    const auto& pMatInverse = cam.getPMatInverse();
    const auto& vMat = cam.getVMat();
    const auto& MVP = cam.getMVP();
    const auto& MVPInverse = cam.getMVPInverse();
    glm::vec4 nearPlane = pMatInverse * glm::vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

    const float innerRadiusScale = 1.f;
    static float outerRadiusScale = 2.5f;
    static float smoothing = 0.04f;

    // Gui:
    if (ImGui::BeginMenu("Scene")) {
        ImGui::SliderFloat("Radius", &outerRadiusScale, 0.f, 10.f);
        ImGui::SliderFloat("Smoothing Factor", &smoothing, 0.f, 4.f);

        ImGui::EndMenu();
    }
    

    // Sphere pass
    {
        auto g = sphereFramebuffer->guard();
        glClearColor(0.f, 0.f, 0.f, -1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!shaders.contains("sphere"))
            return;
        
        const auto shaderId = *shaders.at("sphere");
        glUseProgram(shaderId);
        uniform(shaderId, "MVP", MVP);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "modelViewMatrix", vMat);
        uniform(shaderId, "projectionMatrix", pMat);
        uniform(shaderId, "nearPlaneZ", nearPlane.z);
        uniform(shaderId, "time", runningTime);

        auto g2 = sceneBuffer->guard();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(SCENE_SIZE));
    }


    // List pass
    {
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);        
        
        if (!shaders.contains("list"))
            return;
            
        const auto shaderId = *shaders.at("list");
        glUseProgram(shaderId);

        positionTexture->bind();
        listBuffer->bindBase();
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_INT, 0);

        uniform(shaderId, "modelViewMatrix", vMat);
        uniform(shaderId, "projectionMatrix", pMat);
        uniform(shaderId, "MVP", MVP);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "nearPlaneZ", nearPlane.z);
        uniform(shaderId, "radiusScale", outerRadiusScale);

        auto g2 = sceneBuffer->guard();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(SCENE_SIZE));
    }


    // Surface pass
    {
        // glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT​);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!shaders.contains("surface"))
            return;
            
        const auto shaderId = *shaders.at("surface");
        glUseProgram(shaderId);

        auto vao = std::make_unique<VertexArray>();
        glBindBuffer(GL_ARRAY_BUFFER, listBuffer->id); // Bind as VBO
        vao->vertexAttribute(0, 1, GL_UNSIGNED_INT, GL_FALSE); // count
        vao->vertexAttribute(1, 2, GL_UNSIGNED_INT, GL_FALSE); // screenCoord
        for (GLuint i{0}; i < LIST_MAX_ENTRIES; ++i) // pos[MAX_ENTRIES]
            vao->vertexAttribute(2 + i, 4, GL_FLOAT, GL_FALSE);

        positionTexture->bind();
        auto v = vao->guard();
        // listBuffer->bindBase();
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "time", runningTime);
        uniform(shaderId, "smoothing", smoothing);

        glDrawArrays(GL_POINTS, 0, SCR_SIZE.x * SCR_SIZE.y);

        // screenMesh.draw();
    }
}