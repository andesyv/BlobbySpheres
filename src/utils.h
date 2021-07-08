#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "components.h"
#include "zip.hpp"

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

template <GLenum TextureType, typename DimType>
class TextureBase {
public:
    unsigned int id;
    const GLenum type = TextureType;

    TextureBase() {
        glGenTextures(1, &id);
    }

    void data(GLint level, GLint internalformat, DimType size, GLint border, GLenum format, GLenum type, const void * data) = delete;

    void init(DimType size = DimType{}, GLenum format = GL_RGBA16F) = delete;

    void bind(unsigned int textureUnit = 0) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(TextureType, id);
    }

    ~TextureBase() {
        glDeleteTextures(1, &id);
    }
};

template <GLenum TextureType>
class Texture : public TextureBase<TextureType, void> {};

// template <>
// class Texture<GL_TEXTURE_1D> : public TextureBase<GL_TEXTURE_1D, glm::vec1> {
// public:
//     void textureData(GLint level, GLint internalformat, float size, GLint border, GLenum format, GLenum type, const void * data) {
//         glTexImage1D(GL_TEXTURE_1D, level, internalformat, size, border, format, type, data);
//     }

//     void init() {

//     }
// };

template <>
class Texture<GL_TEXTURE_2D> : public TextureBase<GL_TEXTURE_2D, glm::ivec2> {
public:
    void data(GLint level, GLint internalformat, glm::ivec2 size, GLint border, GLenum format, GLenum type, const void * data) {
        glTexImage2D(GL_TEXTURE_2D, level, internalformat, size.x, size.y, border, format, type, data);
    }

    void init(glm::ivec2 size = glm::ivec2{}, GLenum format = GL_RGBA16F) {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        data(0, format, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    Texture() : TextureBase<GL_TEXTURE_2D, glm::ivec2>{} {}
    Texture(glm::ivec2 size, GLenum format = GL_RGBA16F) : TextureBase<GL_TEXTURE_2D, glm::ivec2>{} {
        init(size, format);
    }
};

// template <>
// class Texture<GL_TEXTURE_3D> : public TextureBase<GL_TEXTURE_3D, glm::vec3> {
// public:
//     void textureData(GLint level, GLint internalformat, glm::vec3 size, GLint border, GLenum format, GLenum type, const void * data) {
//         glTexImage3D(GL_TEXTURE_3D, level, internalformat, size.x, size.y, size.z, border, format, type, data);
//     }

//     void init() {

//     }
// };

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

class Framebuffer {
private:
    bool bValid = false;

public:
    unsigned int id;
    std::vector<std::shared_ptr<Texture<GL_TEXTURE_2D>>> textures;
    std::shared_ptr<Texture<GL_TEXTURE_2D>> depthTexture;
    std::shared_ptr<Texture<GL_TEXTURE_2D>> stencilTexture;
    std::vector<GLenum> rendertargets;

    // template <GLenum TextureType>
    void addTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>>&& texture) {
        textures.push_back(std::move(texture));
    }

    void addDepthTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>>&& texture) {
        depthTexture = std::move(texture);
    }

    void addStencilTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>>&& texture) {
        stencilTexture = std::move(texture);
    }

    bool assemble();

    Framebuffer() {
        glGenFramebuffers(1, &id);
    }

    template <std::size_t I>
    Framebuffer(std::pair<GLenum, std::shared_ptr<Texture<GL_TEXTURE_2D>>> (&& params)[I]) {
        glGenFramebuffers(1, &id);

        for (auto [target, texture] : params) {
            if (target == GL_DEPTH_ATTACHMENT)
                addDepthTexture(std::move(texture));
            else if (target == GL_STENCIL_ATTACHMENT)
                addStencilTexture(std::move(texture));
            else {
                addTexture(std::move(texture));
                rendertargets.push_back(target);
            }
        }

        bValid = assemble();
    }

    void bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void bindDraw() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); }
    void bindRead() { glBindFramebuffer(GL_READ_FRAMEBUFFER, id); }
    void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    auto guard() { return Guard{this}; }

    bool valid() const { return bValid; }
    std::string completeness();

    ~Framebuffer() {
        glDeleteFramebuffers(1, &id);
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