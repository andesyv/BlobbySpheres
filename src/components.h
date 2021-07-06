#pragma once
#include <glm/glm.hpp>
#include "utils.h"

struct Transform {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct Mesh {
    std::weak_ptr<util::VertexArray> vao;
    unsigned int vertexCount{0}, indexCount{0};

    Mesh() = default;
    Mesh(std::shared_ptr<util::VertexArray>& vertexArray, unsigned int vCount = 0, unsigned int iCount = 0)
        : vao{vertexArray}, vertexCount{vCount}, indexCount{iCount}
    {}

    void draw() {
        if (vao.expired())
            return;

        auto vao_ptr = vao.lock();
            
        const auto guard = vao_ptr->guard();
        if (vao_ptr->hasIndices())
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
};

struct Material {
    int shader;
};