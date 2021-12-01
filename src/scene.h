#ifndef SCENE_H
#define SCENE_H

#include "shader.h"
#include "components.h"
#include "utils.h"
#include "globjects.h"

#include <map>
#include <array>
#include <entt/entt.hpp>

class Scene {
private:
    std::map<std::string, Shader> shaders;
    comp::Mesh screenMesh;
    entt::registry EM;
    std::shared_ptr<globjects::Buffer<GL_SHADER_STORAGE_BUFFER>> sceneBuffer, sceneBuffer2;

    std::shared_ptr<globjects::Tex2D> positionTexture, depthTexture;

    std::shared_ptr<globjects::Buffer<GL_SHADER_STORAGE_BUFFER>> listBuffer;
    std::shared_ptr<globjects::Tex2D> listIndexTexture, listIndexTexture2;
    std::unique_ptr<globjects::Tex3D> volumeSampleTexture, volumeSampleTexture2;

    std::vector<glm::vec4> positions, positions2;

public:
    Scene();

    void reloadShaders();

    void render(float deltaTime = 0.f);
    void animate(float deltaTime = 0.f);
};

#endif // SCENE_H