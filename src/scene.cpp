#include "scene.h"
#include "utils.h"
#include "settings.h"
#include "camera.h"
#include "constants.h"

// #include <format>
#include <vector>
#include <iostream>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

using namespace comp;
using namespace globjects;
using namespace util;

constexpr glm::uint SCENE_SIZE = 1000;
constexpr glm::uint SCENE_SIZE2 = SCENE_SIZE / 7;
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
            util::format("MAX_ENTRIES {}u", MAX_ENTRIES),
            util::format("LIST_MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
            util::format("SCREEN_SIZE uvec2({},{})", SCR_SIZE.x, SCR_SIZE.y)
        }
    }));

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            util::format("SCENE_SIZE {}u", SCENE_SIZE),
            util::format("MAX_ENTRIES {}u", MAX_ENTRIES),
            util::format("LIST_MAX_ENTRIES {}u", LIST_MAX_ENTRIES),
            util::format("SCREEN_SIZE uvec2({},{})", SCR_SIZE.x, SCR_SIZE.y)
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
    positions.reserve(SCENE_SIZE);

    for (glm::uint i = 0; i < SCENE_SIZE; ++i) {
        auto entity = EM.create();

        const auto pos = glm::ballRand(0.5f);
        const auto radius = glm::linearRand(0.01f, 0.1f);
        const auto mass = 10.f * radius * radius;
        auto velocity = glm::normalize(randomDiskPoint(pos, 1.f) - pos) * glm::linearRand(0.1f, 0.5f);
        // Multiply with mean orbital speed (https://en.wikipedia.org/wiki/Orbital_speed#Mean_orbital_speed):
        // velocity *= std::sqrt((G * PHYSICS_CENTER_MASS) / glm::length(pos));

        EM.emplace<Sphere>(entity, pos, radius, 0u);
        EM.emplace<Physics>(entity, velocity, mass);
        positions.emplace_back(pos, radius);
    }

    sceneBuffer = std::make_shared<VertexArray>(positions, GL_DYNAMIC_DRAW);
    sceneBuffer->vertexAttribute(0, 4, GL_FLOAT, GL_FALSE);


    positions2.reserve(SCENE_SIZE2);
    for (glm::uint i = 0; i < SCENE_SIZE2; ++i) {
        auto entity = EM.create();

        const auto pos = glm::ballRand(0.4f);
        const auto radius = glm::linearRand(0.01f, 0.2f);
        const auto mass = 10.f * radius * radius;
        auto velocity = glm::normalize(randomDiskPoint(pos, 1.f) - pos) * glm::linearRand(1.f, 2.f);

        EM.emplace<Sphere>(entity, pos, radius, 1u);
        EM.emplace<Physics>(entity, velocity, mass);
        positions2.emplace_back(pos, radius);
    }

    sceneBuffer2 = std::make_shared<VertexArray>(positions2, GL_DYNAMIC_DRAW);
    sceneBuffer2->vertexAttribute(0, 4, GL_FLOAT, GL_FALSE);

    // Framebuffers:
    positionTexture = std::make_shared<Tex2D>(SCR_SIZE);
    positionTexture2 = std::make_shared<Tex2D>(SCR_SIZE);
    normalTexture = std::make_shared<Tex2D>(SCR_SIZE);
    normalTexture2 = std::make_shared<Tex2D>(SCR_SIZE);
    depthTexture = std::make_shared<Tex2D>(SCR_SIZE, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

    sphereFramebuffer = std::make_shared<Framebuffer>(
        std::make_pair(GL_COLOR_ATTACHMENT0, std::shared_ptr{positionTexture}),
        std::make_pair(GL_COLOR_ATTACHMENT1, std::shared_ptr{normalTexture}),
        std::make_pair(GL_DEPTH_ATTACHMENT, std::shared_ptr{depthTexture})
    );
    sphereFramebuffer2 = std::make_shared<Framebuffer>(
        std::make_pair(GL_COLOR_ATTACHMENT0, std::shared_ptr{positionTexture2}),
        std::make_pair(GL_COLOR_ATTACHMENT1, std::shared_ptr{normalTexture2}),
        std::make_pair(GL_DEPTH_ATTACHMENT, std::shared_ptr{depthTexture})
    );
    
    assert(sphereFramebuffer->completeness() == "GL_FRAMEBUFFER_COMPLETE");
    assert(sphereFramebuffer2->completeness() == "GL_FRAMEBUFFER_COMPLETE");


    listIndexTexture = std::make_shared<Tex2D>(SCR_SIZE, GL_R32UI, GL_RED_INTEGER);
    listIndexTexture2 = std::make_shared<Tex2D>(SCR_SIZE, GL_R32UI, GL_RED_INTEGER);

    // SSBOs
    const std::size_t entrySize = sizeof(glm::vec4);
    const std::size_t bufferSize = entrySize * MAX_ENTRIES * 2 * SCR_SIZE.x * SCR_SIZE.y;
    listBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(bufferSize, GL_DYNAMIC_DRAW);
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << util::format("Reloading \"{}\" shader failed!", name) << std::endl;
}

void Scene::render(float deltaTime) {
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
    static float interpolation = 0.0f;
    static bool animation = false;
    static float animationSpeed = 1.f;

    // Gui:
    if (ImGui::BeginMenu("Scene")) {
        ImGui::SliderFloat("Radius", &outerRadiusScale, 0.f, 10.f);
        ImGui::SliderFloat("Smoothing Factor", &smoothing, 0.f, 4.f);
        ImGui::SliderFloat("Interpolation", &interpolation, 0.f, 1.f);
        ImGui::Checkbox("Animation", &animation);
        if (animation)
            ImGui::DragFloat("Animation speed", &animationSpeed, 0.1f, 0.1f, 10.f);

        ImGui::EndMenu();
    }

    // Clear buffers:
    {
        listBuffer->bind();
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_INT, 0);
        // listIndexTexture->clear();
        const static auto intClearData = gen_vec(600*800, 0u);
        glActiveTexture(GL_TEXTURE0);
        listIndexTexture->bind();
        listIndexTexture->data(0, GL_R32UI, Settings::get().SCR_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, intClearData.data());
        listIndexTexture2->bind();
        listIndexTexture2->data(0, GL_R32UI, Settings::get().SCR_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, intClearData.data());
    }


    // Sphere pass
    {
        glClearColor(0.f, 0.f, 0.f, FAR_DIST);
        glEnable(GL_DEPTH_TEST);

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

        for (glm::uint i{0}; i < 2; ++i) {
            auto g = (i == 0 ? sphereFramebuffer : sphereFramebuffer2)->guard();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto g2 = (i == 0 ? sceneBuffer : sceneBuffer2)->guard();
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(i == 0 ? SCENE_SIZE : SCENE_SIZE2));
        }
    }


    // List pass
    {
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        
        if (!shaders.contains("list"))
            return;
            
        const auto shaderId = *shaders.at("list");
        glUseProgram(shaderId);
        uniform(shaderId, "modelViewMatrix", vMat);
        uniform(shaderId, "projectionMatrix", pMat);
        uniform(shaderId, "MVP", MVP);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "clipNearPlaneZ", clipNearPlane.z);
        uniform(shaderId, "radiusScale", outerRadiusScale);

        listBuffer->bindBase(0);

        for (glm::uint i{0}; i < 2; ++i) {
            uniform(shaderId, "passIndex", i);

            (i == 0 ? positionTexture : positionTexture2)->bind(0);

            glBindImageTexture(1, (i == 0 ? listIndexTexture : listIndexTexture2)->id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

            auto g2 = (i == 0 ? sceneBuffer : sceneBuffer2)->guard();
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(i == 0 ? SCENE_SIZE : SCENE_SIZE2));
        }
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

        // positionTexture->bind(0);
        listIndexTexture->bind(1);
        // positionTexture2->bind(2);
        listIndexTexture2->bind(3);
        listBuffer->bindBase(0);
        uniform(shaderId, "MVPInverse", MVPInverse);
        uniform(shaderId, "time", runningTime);
        uniform(shaderId, "smoothing", smoothing);
        uniform(shaderId, "interpolation", interpolation);

        screenMesh.draw();
    }

    if (animation)
        animate(deltaTime * animationSpeed);
}

void Scene::animate(float deltaTime) {
    std::size_t i{0}, j{0};
    const auto view = EM.view<Sphere, Physics>();

    for (auto entity : view) {
        auto [trans, phys] = view.get<Sphere, Physics>(entity);

        /// "Faking" gravity
        const auto dir = phys.velocity;
        const auto rotDir = glm::normalize(glm::cross(dir, trans.pos));
        const auto rotAmount = glm::length(dir) * deltaTime;
        const auto rotation = glm::angleAxis(rotAmount, rotDir);
        
        trans.pos = rotation * trans.pos;
        phys.velocity = rotation * phys.velocity;

        // const float radius = glm::length(trans.pos);
        // const auto dir = (trans.pos / radius);
        // const auto F = dir * ((G * phys.mass * PHYSICS_CENTER_MASS) / (radius * radius));
        
        // phys.velocity += (F / phys.mass) * deltaTime;
        // trans.pos += phys.velocity * deltaTime;

        if (trans.LOD == 0u) {
            positions[i] = glm::vec4{trans.pos, trans.radius};
            ++i;
        } else {
            positions2[j] = glm::vec4{trans.pos, trans.radius};
            ++j;
        }
    }

    sceneBuffer->vertexBuffer->updateBuffer(positions);
    sceneBuffer2->vertexBuffer->updateBuffer(positions2);
}