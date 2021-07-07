#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>

namespace util {
    class VertexArray;
};

namespace comp {
struct Transform {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct Mesh {
    std::weak_ptr<util::VertexArray> vao;
    unsigned int vertexCount{0}, indexCount{0};

    Mesh() = default;
    Mesh(std::shared_ptr<util::VertexArray>& vertexArray, unsigned int vCount = 0, unsigned int iCount = 0);

    void draw();
};

struct Material {
    int shader;

    std::map<std::string, unsigned int> cachedUniformLocations;
};

}