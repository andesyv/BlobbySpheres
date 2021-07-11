#ifndef SCENE_H
#define SCENE_H

#include "shader.h"
#include "components.h"

#include <map>
#include <array>
#include <entt/entt.hpp>

namespace util {
    class VertexArray;
}

class Scene {
public:
    static constexpr std::size_t SCENE_SIZE = 100u;
    glm::mat4 MVPInverse;

private:
    std::map<std::string, Shader> shaders;
    comp::Mesh screenMesh;
    entt::registry EM;
    std::array<glm::vec4, SCENE_SIZE> positions;
    std::unique_ptr<util::VertexArray> sceneBuffer;

public:
    Scene(const glm::mat4& mvpinv = glm::mat4{1.f});

    void render();
};

#endif // SCENE_H