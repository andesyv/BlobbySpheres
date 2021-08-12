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
using namespace globjects;
using namespace util;

constexpr glm::uint SCENE_SIZE = 1000;
constexpr std::size_t MAX_ENTRIES = 32u;
constexpr std::size_t LIST_MAX_ENTRIES = MAX_ENTRIES * 800 * 600;
constexpr float FAR_DIST = 1000.f;

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
            std::format("MAX_ENTRIES {}u", MAX_ENTRIES),
            std::format("LIST_MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
            std::format("SCREEN_SIZE uvec2({},{})", SCR_SIZE.x, SCR_SIZE.y)
        }
    }));

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE),
            std::format("MAX_ENTRIES {}u", MAX_ENTRIES),
            std::format("LIST_MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
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
    std::vector<glm::vec4> positions;
    positions.reserve(SCENE_SIZE);

    for (glm::uint i = 0; i < SCENE_SIZE; ++i) {
        auto entity = EM.create();

        const auto pos = glm::ballRand(0.5f);
        const auto radius = glm::linearRand(0.01f, 0.1f);

        EM.emplace<Sphere>(entity, pos, radius);
        positions.emplace_back(pos, radius);
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


    listIndexTexture = std::make_shared<Tex2D>(SCR_SIZE, GL_R32UI, GL_RED_INTEGER);

    // SSBOs
    const std::size_t entrySize = sizeof(glm::vec4);
    const std::size_t bufferSize = entrySize * MAX_ENTRIES * SCR_SIZE.x * SCR_SIZE.y;
    listBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(bufferSize, GL_DYNAMIC_DRAW);
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << std::format("Reloading \"{}\" shader failed!", name) << std::endl;
}

void Scene::render() {
    const auto runningTime = Settings::get().runningTime;
    const auto& cam = Camera::getGlobalCamera();
    const auto& pMat = cam.getPMat();
    const auto& pMatInverse = cam.getPMatInverse();
    const auto& vMat = cam.getVMat();
    const auto& MVP = cam.getMVP();
    const auto& MVPInverse = cam.getMVPInverse();
    glm::vec4 clipNearPlane = pMatInverse * glm::vec4(0.0, 0.0, -1.0, 1.0);
	clipNearPlane /= clipNearPlane.w;

    const float innerRadiusScale = 1.f;
    static float outerRadiusScale = 2.5f;
    static float smoothing = 0.04f;

    // Gui:
    if (ImGui::BeginMenu("Scene")) {
        ImGui::SliderFloat("Radius", &outerRadiusScale, 0.f, 10.f);
        ImGui::SliderFloat("Smoothing Factor", &smoothing, 0.f, 4.f);

        ImGui::EndMenu();
    }

    // Clear buffers:
    {
        auto g = listBuffer->guard();
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_INT, 0);
        // listIndexTexture->clear();
        glActiveTexture(GL_TEXTURE0);
        listIndexTexture->bind();
        const static auto intClearData = gen_vec(600*800, 0u);
        listIndexTexture->data(0, GL_R32UI, Settings::get().SCR_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, intClearData.data());
    }


    // Sphere pass
    {
        auto g = sphereFramebuffer->guard();
        glClearColor(0.f, 0.f, 0.f, FAR_DIST);
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
        uniform(shaderId, "clipNearPlaneZ", clipNearPlane.z);
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

        positionTexture->bind(0);

        glBindImageTexture(1, listIndexTexture->id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        listBuffer->bindBase();
        uniform(shaderId, "modelViewMatrix", vMat);
        uniform(shaderId, "projectionMatrix", pMat);
        uniform(shaderId, "MVP", MVP);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "clipNearPlaneZ", clipNearPlane.z);
        uniform(shaderId, "radiusScale", outerRadiusScale);

        auto g2 = sceneBuffer->guard();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(SCENE_SIZE));
    }


    // Surface pass
    {
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!shaders.contains("surface"))
            return;
            
        const auto shaderId = *shaders.at("surface");
        glUseProgram(shaderId);

        // positionTexture->bind();
        listIndexTexture->bind(1);
        listBuffer->bindBase();
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "time", runningTime);
        uniform(shaderId, "smoothing", smoothing);

        screenMesh.draw();
    }
}