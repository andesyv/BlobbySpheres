#include "scene.h"
#include "utils.h"
#include "settings.h"
#include "camera.h"
#include "constants.h"

#include <format>
#include <vector>
#include <iostream>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

using namespace comp;
using namespace globjects;
using namespace util;

constexpr glm::uint SCENE_SIZE = 10;
constexpr glm::uint SCENE_SIZE2 = 40;
constexpr float FAR_DIST = 1000.f;
constexpr glm::uint UNIFORM_GRID_SIZE = 100; // 1000^3 * float = 1GB! - and 1000000000 threads :s

Scene::Scene()
{
    const auto SCR_SIZE = Settings::get().SCR_SIZE;

    shaders.insert(std::make_pair("surface", Shader{
        {
            {GL_VERTEX_SHADER, "screen.vert.glsl"},
            {GL_FRAGMENT_SHADER, "sdf.frag.glsl"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE)
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

        const auto pos = glm::ballRand(0.5f);
        const auto radius = glm::linearRand(0.01f, 0.1f);
        const auto mass = 10.f * radius * radius;
        auto velocity = glm::normalize(randomDiskPoint(pos, 1.f) - pos) * glm::linearRand(0.1f, 0.5f);
        // Multiply with mean orbital speed (https://en.wikipedia.org/wiki/Orbital_speed#Mean_orbital_speed):
        // velocity *= std::sqrt((G * PHYSICS_CENTER_MASS) / glm::length(pos));

        EM.emplace<Sphere>(entity, pos, radius, 0u);
        EM.emplace<Physics>(entity, velocity, mass);
        positions2.emplace_back(pos, radius);
    }

    sceneBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(positions, GL_DYNAMIC_DRAW);
    sceneBuffer2 = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(positions2, GL_DYNAMIC_DRAW);

    volumeSampleTexture = std::make_unique<Tex3D>(glm::ivec3{UNIFORM_GRID_SIZE}, GL_R16F, GL_RED);
    volumeSampleTexture2 = std::make_unique<Tex3D>(glm::ivec3{UNIFORM_GRID_SIZE}, GL_R16F, GL_RED);
}

void Scene::reloadShaders() {
    std::cout << "Reloading shaders!" << std::endl;

    for (auto& [name, program] : shaders)
        if (!program.reload())
            std::cout << std::format("Reloading \"{}\" shader failed!", name) << std::endl;
}

void Scene::render(float deltaTime) {
    const auto& cam = Camera::getGlobalCamera();
    const auto& MVPInverse = cam.getMVPInverse();

    static bool animation = false;
    static float animationSpeed = 1.f;

    // Gui:
    if (ImGui::BeginMenu("Scene")) {
        ImGui::Checkbox("Animation", &animation);
        if (animation)
            ImGui::DragFloat("Animation speed", &animationSpeed, 0.1f, 0.1f, 10.f);

        ImGui::EndMenu();
    }

    // Displacement sampling pass
    {
        if (!shaders.contains("sample-grid"))
            return;

        const auto shaderId = *shaders.at("sample-grid");
        glUseProgram(shaderId);

        volumeSampleTexture->clear();
        // TODO: Need to bind texture with glBindImageTexture instead to make it writable in the shader.
        auto guards = std::make_tuple(volumeSampleTexture->guard(0), volumeSampleTexture2->guard(1), sceneBuffer->guard(2), sceneBuffer2->guard(3));

        glDispatchCompute(UNIFORM_GRID_SIZE, UNIFORM_GRID_SIZE, UNIFORM_GRID_SIZE);

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
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

        sceneBuffer->bindBase(0);

        uniform(shaderId, "MVPInverse", MVPInverse);

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

        positions[i] = glm::vec4{trans.pos, trans.radius};
        ++i;
    }

    sceneBuffer->updateBuffer(positions);
}