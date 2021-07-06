#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <bitset>

namespace util {
// Util guard class
template <typename T>
class Guard {
private:
    T* mPtr;

public:
    Guard(T* ptr) : mPtr{ptr} {
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
    Buffer(const std::vector<T>& params) {
        glGenBuffers(1, &id);
        glBindBuffer(BufferType, id);
        glBufferData(BufferType, sizeof(T) * params.size(), params.data(), GL_STATIC_DRAW);
        glBindBuffer(BufferType, 0);
    }

    void bind() {
        glBindBuffer(BufferType, id);
    }

    void unbind() {
        glBindBuffer(BufferType, 0);
    }

    auto guard() { return Guard{this}; }

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

// template <typename T>
// using Buffer = std::unique_ptr<T>;
}