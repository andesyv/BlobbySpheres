#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>

namespace globjects {
    class VertexArray;
};

namespace comp {
struct Transform {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct Sphere {
    glm::vec3 pos;
    float radius;
    glm::uint LOD;
};

struct Physics {
    glm::vec3 velocity;
    float mass;
};

struct Mesh {
    std::shared_ptr<globjects::VertexArray> vao;
    unsigned int vertexCount{0}, indexCount{0};

    Mesh() = default;
    Mesh(std::shared_ptr<globjects::VertexArray>& vertexArray, unsigned int vCount = 0, unsigned int iCount = 0);

    void draw();
};

struct Material {
    int shader;

    std::map<std::string, unsigned int> cachedUniformLocations;
};

}

#endif // COMPONENTS_H