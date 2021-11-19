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
constexpr float FAR_DIST = 1000.f;

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

    sceneBuffer = std::make_shared<Buffer<GL_SHADER_STORAGE_BUFFER>>(positions, GL_DYNAMIC_DRAW);
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