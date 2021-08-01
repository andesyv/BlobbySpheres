#ifndef SCENE_H
#define SCENE_H

#include "shader.h"
#include "components.h"
#include "utils.h"

#include <map>
#include <array>
#include <entt/entt.hpp>

class Scene {
private:
    std::map<std::string, Shader> shaders;
    comp::Mesh screenMesh;
    entt::registry EM;
    std::unique_ptr<util::VertexArray> sceneBuffer;

    std::shared_ptr<util::Tex2D> positionTexture, normalTexture, depthTexture;
    std::shared_ptr<util::Framebuffer<util::Tex2D>> sphereFramebuffer;
    std::shared_ptr<util::Framebuffer<util::Tex3D>> listFramebuffer;

    std::shared_ptr<util::Buffer<GL_SHADER_STORAGE_BUFFER>> listBuffer;
    std::shared_ptr<util::Tex3D> listTextureBuffer;
    std::shared_ptr<util::Tex2D> listIndexBuffer;

public:
    Scene();

    void reloadShaders();

    void render();
};

#endif // SCENE_H