#include "scene.h"
#include "utils.h"
#include "settings.h"
#include "camera.h"
#include "constants.h"

#include <format>
#include <vector>
#include <iostream>
#include <memory>

#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

using namespace comp;
using namespace globjects;
using namespace util;

constexpr glm::uint SCENE_SIZE = 10;
constexpr glm::uint SCENE_SIZE2 = 3;
constexpr float FAR_DIST = 1000.f;
constexpr glm::uvec3 UNIFORM_GRID_SIZE = { 300, 300, 300 };

Scene::Scene()
{
    const auto SCR_SIZE = Settings::get().SCR_SIZE;

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE),
            std::format("SCENE_SIZE2 {}u", SCENE_SIZE2),
        }
    }));

    shaders.insert(std::make_pair("sample-grid", Shader{
        {
            {GL_COMPUTE_SHADER, "sample-grid.comp.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE),
            std::format("SCENE_SIZE2 {}u", SCENE_SIZE2),
        }
    }));

    std::erase_if(shaders, [](const auto& item){ return !item.second.valid(); });


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

    for (glm::uint i = 0; i < SCENE_SIZE2; ++i) {
        auto entity = EM.create();

        const auto pos = glm::ballRand(0.3f);
        const auto radius = glm::linearRand(0.02f, 0.2f);
        const auto mass = 10.f * radius * radius;
        auto velocity = glm::normalize(randomDiskPoint(pos, 1.f) - pos) * glm::linearRand(0.1f, 0.5f);
        // Multiply with mean orbital speed (https://en.wikipedia.org/wiki/Orbital_speed#Mean_orbital_speed):
        // velocity *= std::sqrt((G * PHYSICS_CENTER_MASS) / glm::length(pos));

        EM.emplace<Sphere>(entity, pos, radius, 1u);
        EM.emplace<Physics>(entity, velocity, mass);
        positions2.emplace_back(pos, radius);
    }

    sceneBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(positions, GL_DYNAMIC_DRAW);
    sceneBuffer2 = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(positions2, GL_DYNAMIC_DRAW);

    volumeDiffTexture = std::make_unique<Tex3D>(glm::ivec3{UNIFORM_GRID_SIZE}, GL_R16F, GL_RED);
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << std::format("Reloading \"{}\" shader failed!", name) << std::endl;
}

void Scene::render(float deltaTime) {
    const auto& cam = Camera::getGlobalCamera();
    const auto& MVP = cam.getMVP();
    const auto& MVPInverse = cam.getMVPInverse();

    static bool animation = false;
    static float animationSpeed = 1.f;
    static float interpolation = 0.f;

    // Gui:
    if (ImGui::BeginMenu("Scene")) {
        ImGui::Checkbox("Animation", &animation);
        if (animation)
            ImGui::DragFloat("Animation speed", &animationSpeed, 0.1f, 0.1f, 10.f);
        ImGui::SliderFloat("Interpolation", &interpolation, 0.f, 10.f);

        ImGui::EndMenu();
    }

    /** Plan:
     * 1. Run compute shaders on uniform grid, set on scene. For each invocation:
     * 2. Sample SDF for both LODs
     * 3. Save the difference from the first LOD to the second as a gradient (LOD1 - LOD2 = diff)
     * 4. After sample step, use this difference field as a displacement volume for the rendering.
     */

    // Displacement sampling pass
    {
        if (!shaders.contains("sample-grid") && shaders.at("sample-grid").valid())
            return;

        const auto shaderId = *shaders.at("sample-grid");
        glUseProgram(shaderId);

//        volumeDiffTexture->clear();
        auto guards = std::make_tuple(sceneBuffer->guard(2), sceneBuffer2->guard(3));
        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, volumeDiffTexture->id, 0, false, 0, GL_READ_WRITE, GL_R16F);

        uniform(shaderId, "MVPInverse", MVPInverse);

        glDispatchCompute(UNIFORM_GRID_SIZE.x, UNIFORM_GRID_SIZE.y, UNIFORM_GRID_SIZE.z);

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // Surface pass
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!shaders.contains("surface") && shaders.at("surface").valid())
            return;

        const auto shaderId = *shaders.at("surface");
        glUseProgram(shaderId);

        auto guards = std::make_tuple(sceneBuffer->guard(0), sceneBuffer2->guard(1), volumeDiffTexture->guard(2));

        uniform(shaderId, "MVP", MVP);
        uniform(shaderId, "MVPInverse", MVPInverse);
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
        if (trans.LOD != 0u) continue;

        /// "Faking" gravity
        const auto dir = phys.velocity;
        const auto rotDir = glm::normalize(glm::cross(dir, trans.pos));
        const auto rotAmount = glm::length(dir) * deltaTime;
        const auto rotation = glm::angleAxis(rotAmount, rotDir);
        
        trans.pos = rotation * trans.pos;
        phys.velocity = rotation * phys.velocity;

        positions.at(i) = glm::vec4{trans.pos, trans.radius};
        ++i;
    }

    sceneBuffer->updateBuffer(positions);
}