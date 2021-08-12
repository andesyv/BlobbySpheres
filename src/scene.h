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
    std::shared_ptr<globjects::VertexArray> sceneBuffer, sceneBuffer2;

    std::shared_ptr<globjects::Tex2D> positionTexture, normalTexture, positionTexture2, normalTexture2, depthTexture;
    std::shared_ptr<globjects::Framebuffer> sphereFramebuffer, sphereFramebuffer2;

    std::shared_ptr<globjects::Buffer<GL_SHADER_STORAGE_BUFFER>> listBuffer;
    std::shared_ptr<globjects::Tex2D> listIndexTexture, listIndexTexture2;

public:
    Scene();

    void reloadShaders();

    void render();
};

#endif // SCENE_H