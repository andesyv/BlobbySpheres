#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "components.h"

namespace util {
// Util guard class
template <typename T>
class Guard {
protected:
    T* mPtr;

public:
    Guard(T* ptr)
     : mPtr{ptr} {
        if (mPtr)
            mPtr->bind();
    }

    ~Guard() {
        if (mPtr)
            mPtr->unbind();
    }
};

template <GLenum BufferType>
class Buffer {
public:
    unsigned int id;
    const GLenum type = BufferType;

    template <typename T>
    void bufferData(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
        auto g = guard();
        glBufferData(BufferType, sizeof(T) * data.size(), data.data(), usage);
    }
    
    Buffer() {
        glGenBuffers(1, &id);
    }

    template <typename T>
    Buffer(const std::vector<T>& params, GLenum usage = GL_STATIC_DRAW) {
        glGenBuffers(1, &id);
        bufferData(params, usage);
    }

    void bind() {
        glBindBuffer(BufferType, id);
    }

    void unbind() {
        glBindBuffer(BufferType, 0);
    }

    auto guard() { return Guard{this}; }

    void bindBase(unsigned int binding = 0) {
        glBindBufferBase(BufferType, 0, id);
    }

    ~Buffer() {
        glDeleteBuffers(1, &id);
    }
};

class VertexArray {
private:
    bool bInit = false;

public:
    unsigned int id;
    std::unique_ptr<Buffer<GL_ARRAY_BUFFER>> vertexBuffer;
    std::unique_ptr<Buffer<GL_ELEMENT_ARRAY_BUFFER>> indexBuffer;
    
    template <typename T>
    VertexArray(const std::vector<T>& vertices) : bInit{true} {
        glGenVertexArrays(1, &id);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(id);

        vertexBuffer = std::make_unique<Buffer<GL_ARRAY_BUFFER>>(vertices);
        vertexBuffer->bind();
    }

    template <typename T, std::size_t I>
    VertexArray(T (&& vertices)[I]) : VertexArray{std::vector{vertices}} {}

    template <typename T, typename U>
    VertexArray(const std::vector<T>& vertices, const std::vector<U>& indices) : bInit{true} {
        glGenVertexArrays(1, &id);

        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(id);

        vertexBuffer = std::make_unique<Buffer<GL_ARRAY_BUFFER>>(vertices);
        vertexBuffer->bind();

        indexBuffer = std::make_unique<Buffer<GL_ELEMENT_ARRAY_BUFFER>>(indices);
        indexBuffer->bind();
    }

    void vertexAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer) {
        glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        glEnableVertexAttribArray(index);
    }

    void bind() {
        glBindVertexArray(id);
    }

    void unbind() {
        glBindVertexArray(0);
    }

    auto guard() { return Guard{this}; }

    bool hasIndices() const { return static_cast<bool>(indexBuffer); }

    ~VertexArray() {
        if (bInit)
            glDeleteVertexArrays(1, &id);
    }
};

template <typename T>
void uniform(unsigned int location, const T& value) = delete;

// vec{1-4}
template <> void uniform<float>(unsigned int location, const float& value);
template <> void uniform<glm::vec2>(unsigned int location, const glm::vec2& value);
template <> void uniform<glm::vec3>(unsigned int location, const glm::vec3& value);
template <> void uniform<glm::vec4>(unsigned int location, const glm::vec4& value);

// mat{2-4}
template <> void uniform<glm::mat2>(unsigned int location, const glm::mat2& value);
template <> void uniform<glm::mat3>(unsigned int location, const glm::mat3& value);
template <> void uniform<glm::mat4>(unsigned int location, const glm::mat4& value);

template <typename T>
void uniform(comp::Material& mat, std::string name, const T& value) {
    unsigned int location;
    if (mat.cachedUniformLocations.contains(name)) {
        location = mat.cachedUniformLocations[name];
    } else {
        location = glGetUniformLocation(mat.shader, name.c_str());
        mat.cachedUniformLocations[name] = location;
    }

    uniform(location, value);
}

}