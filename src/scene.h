#ifndef SCENE_H
#define SCENE_H

#include "shader.h"
#include "components.h"
#include "utils.h"

#include <map>
#include <utility>
#include <array>
#include <entt/entt.hpp>

class Scene {
public:
    static constexpr std::size_t SCENE_SIZE = 1000u;

private:
    std::map<std::string, Shader> shaders;
    comp::Mesh screenMesh;
    entt::registry EM;
    std::array<glm::vec4, SCENE_SIZE> positions;
    std::unique_ptr<util::VertexArray> sceneBuffer;

    std::shared_ptr<util::Tex2D> positionTexture, normalTexture, depthTexture;
    std::shared_ptr<util::Framebuffer> sphereFramebuffer;

public:
    Scene();

    void render();
};

#endif // SCENE_H