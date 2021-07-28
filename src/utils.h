#ifndef UTILS_H
#define UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <array>
#include <ranges>
#include <algorithm>

#include "components.h"

typedef std::pair<GLenum, std::string> ESPair;
#define ESTR(x) ESPair{x, #x}

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
    std::size_t bufferSize{0};

    void bufferData(std::size_t byteSize, GLenum usage = GL_STATIC_DRAW) {
        bufferSize = byteSize;
        auto g = guard();
        glBufferData(BufferType, bufferSize, nullptr, usage);
    }

    template <typename T>
    void bufferData(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
        bufferSize = sizeof(T) * data.size();
        auto g = guard();
        glBufferData(BufferType, bufferSize, data.data(), usage);
    }
    
    Buffer() {
        glGenBuffers(1, &id);
    }

    explicit Buffer(std::size_t byteSize, GLenum usage = GL_STATIC_DRAW) {
        glGenBuffers(1, &id);
        bufferData(byteSize, usage);
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

    std::size_t size() const {
        return bufferSize;
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

    virtual void data(GLint level, GLint internalformat, DimType size, GLint border, GLenum format, GLenum type, const void * data) = 0;

    virtual void init(DimType size = DimType{}, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA) = 0;

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
    void data(GLint level, GLint internalformat, glm::ivec2 size, GLint border, GLenum format, GLenum type, const void * data) final {
        glTexImage2D(GL_TEXTURE_2D, level, internalformat, size.x, size.y, border, format, type, data);
    }

    void init(glm::ivec2 size = glm::ivec2{}, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA) final {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        data(0, internalformat, size, 0, format, GL_UNSIGNED_BYTE, nullptr);
    }

    Texture() : TextureBase<GL_TEXTURE_2D, glm::ivec2>{} {}
    Texture(glm::ivec2 size, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA) : TextureBase<GL_TEXTURE_2D, glm::ivec2>{} {
        init(size, internalformat, format);
    }
};

using Tex2D = Texture<GL_TEXTURE_2D>;

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

    explicit VertexArray() : bInit{true} {
        glGenVertexArrays(1, &id);
        glBindVertexArray(id);
    }
    
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

    template <typename T, std::size_t I>
    VertexArray(const std::array<T, I>& vertices) : VertexArray{std::vector{vertices}} {}

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

    void vertexAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride = 0, const void * pointer = nullptr) {
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

    using ConstructionPair = std::pair<GLenum, std::shared_ptr<Texture<GL_TEXTURE_2D>>>;

    template <std::size_t I>
    void constructFromTextures(ConstructionPair (&& params)[I]) {
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

        assemble();
    }

    template <std::size_t I>
    Framebuffer(ConstructionPair (&& params)[I]) {
        glGenFramebuffers(1, &id);

        constructFromTextures<I>(std::move(params));
    }

    template <typename ... T>
    Framebuffer(T&& ... params) {
        glGenFramebuffers(1, &id);

        constexpr auto N = sizeof...(T);
        constructFromTextures<N>({static_cast<ConstructionPair>(params)...});
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

static std::map<int, std::map<std::string, unsigned int>> cachedAnonymousShaderUniformLocations;

template <typename T>
void uniform(int shaderId, std::string name, const T& value) {
    unsigned int location;
    auto& cachedLocations = cachedAnonymousShaderUniformLocations[shaderId];
    if (cachedLocations.contains(name)) {
        location = cachedLocations[name];
    } else {
        location = glGetUniformLocation(shaderId, name.c_str());
        cachedLocations[name] = location;
    }

    uniform(location, value);
}

// https://timur.audio/how-to-make-a-container-from-a-c20-range
template <std::ranges::range R>
auto collect(R&& r) {
    using value_type = std::ranges::range_value_t<decltype(r)>;
    std::vector<value_type> v{};

    if constexpr(std::ranges::sized_range<decltype(r)>)
        v.reserve(std::ranges::size(r));
    
    std::ranges::copy(r, std::back_inserter(v));
    return v;
}

template <typename T>
T getOpenGLParam(GLenum param) = delete;
template <> int getOpenGLParam(GLenum param);
template <> double getOpenGLParam(GLenum param);
template <> float getOpenGLParam(GLenum param);
template <> GLboolean getOpenGLParam(GLenum param);

}

#endif // UTILS_H