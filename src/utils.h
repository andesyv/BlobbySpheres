#ifndef UTILS_H
#define UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <array>
#include <ranges>
#include <algorithm>
#include <sstream>
#include <utility>
#include <tuple>
#include <set>

#include "components.h"

typedef std::pair<GLenum, std::string> ESPair;
#define ESTR(x) ESPair{x, #x}

namespace util {

template<typename What, typename ... Args>
struct is_present {
    static constexpr bool value {(std::is_same_v<What, Args> || ...)};
};

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

    void bufferData(std::size_t byteSize, GLenum usage = GL_STATIC_DRAW) {
        auto g = guard();
        glBufferData(BufferType, byteSize, nullptr, usage);
    }

    template <typename T>
    void bufferData(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
        auto g = guard();
        glBufferData(BufferType, sizeof(T) * data.size(), data.data(), usage);
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

template <>
class Texture<GL_TEXTURE_3D> : public TextureBase<GL_TEXTURE_3D, glm::ivec3> {
public:
    void data(GLint level, GLint internalformat, glm::ivec3 size, GLint border, GLenum format, GLenum type, const void * data) final {
        glTexImage3D(GL_TEXTURE_3D, level, internalformat, size.x, size.y, size.z, border, format, type, data);
    }

    void init(glm::ivec3 size = glm::ivec3{}, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA) final {
        bind();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        data(0, internalformat, size, 0, format, GL_UNSIGNED_BYTE, nullptr);
    }

    Texture() : TextureBase<GL_TEXTURE_3D, glm::ivec3>{} {}
    Texture(glm::ivec3 size, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA) : TextureBase<GL_TEXTURE_3D, glm::ivec3>{} {
        init(size, internalformat, format);
    }
};

using Tex3D = Texture<GL_TEXTURE_3D>;

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

template <class T>
concept TextureType = requires(T t) {
    { Texture{t} } -> std::same_as<T>;
};





template <typename T>
concept FBBufferType = TextureType<T> /* || RenderBufferType<T>*/;

template <typename T>
using FBConstructionPair = std::pair<GLenum, std::shared_ptr<T>>;

template <FBBufferType ... ColorBufferTypes>
class Framebuffer {
private:
    bool bValid = false;

    template <FBBufferType T>
    void bindTexture(const std::pair<std::shared_ptr<T>, GLenum>& arg) {
        auto [tex, target] = arg;
        if constexpr (std::is_same_v<T, Tex2D>)
            glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, tex->id, 0);
        else if constexpr (std::is_same_v<T, Tex3D>)
            glFramebufferTexture3D(GL_FRAMEBUFFER, target, GL_TEXTURE_3D, tex->id, 0, 0);
    }

public:
    template <typename T>
    using BufferList = std::vector<std::pair<std::shared_ptr<T>, GLenum>>;

    unsigned int id;
    std::tuple<BufferList<ColorBufferTypes>...> colorBuffers;
    std::shared_ptr<Texture<GL_TEXTURE_2D>> depthTexture;
    std::shared_ptr<Texture<GL_TEXTURE_2D>> stencilTexture;

    auto getColorBufferCount() const {
        return std::apply([](auto ...list){ return (list.size() + ...); }, colorBuffers);
    }

    std::set<GLenum> getRendertargets() const {
        return std::apply([](auto ...list){
            return collect_set((list | std::views::transform([](const auto& v){ return v.second; }))...);
        }, colorBuffers);
    }

    template <FBBufferType T>
    auto& addColorBuffer(std::shared_ptr<T>&& texture, GLenum target = GL_COLOR_ATTACHMENT0) {
        auto& bufferList = std::get<BufferList<T>>(colorBuffers);
        return bufferList.emplace_back(std::move(texture), target);
    }

    void addDepthTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>>&& texture) {
        depthTexture = std::move(texture);
    }

    void addStencilTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>>&& texture) {
        stencilTexture = std::move(texture);
    }

    bool assemble() {
        // Atleast one color and depth attachment is required by OpenGL
        if (!depthTexture)
            return false;

        auto g = guard();

        // Find rendertargets (if they aren't specified):
        auto targets = getRendertargets();
        if (targets.empty())
            return false;
        else if (targets.size() != getColorBufferCount()) {
            unsigned int i{0};
            std::apply([&](auto& ...list){
                ([&](auto& l){
                    for (auto& v : l)
                        v.second = GL_COLOR_ATTACHMENT0 + (++i - 1);
                }(list),...);
            }, colorBuffers);
            targets = getRendertargets();
        }

        // Bind buffers:
        std::apply([&](const auto& ...list){
            ([&](const auto& l){
                for (const auto& v : l)
                    bindTexture(v);
            }(list),...);
        }, colorBuffers);

        if (depthTexture)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->id, 0);

        if (stencilTexture)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTexture->id, 0);

        // Set draw buffers
        const std::vector<GLenum> targetList{targets.begin(), targets.end()};
        glDrawBuffers(targetList.size(), reinterpret_cast<unsigned int const*>(targetList.data()));

        // Return status
        bValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        return bValid;
    }

    Framebuffer() {
        glGenFramebuffers(1, &id);
    }

    template <FBBufferType T>
    void addBuffer(FBConstructionPair<T>&& param) {
        auto [target, texture] = param;
        addColorBuffer<T>(std::move(texture), target);
    }

    template <>
    void addBuffer(FBConstructionPair<Tex2D>&& param) {
        auto [target, texture] = param;

        if (target == GL_DEPTH_ATTACHMENT)
            addDepthTexture(std::move(texture));
        else if (target == GL_STENCIL_ATTACHMENT)
            addStencilTexture(std::move(texture));
        // Enable this part only if we have a Tex2D buffer in the available buffers:
        else if constexpr ((std::is_same_v<Tex2D, ColorBufferTypes> || ...)) { // https://stackoverflow.com/questions/2118541/check-if-parameter-pack-contains-a-type
            addColorBuffer<Tex2D>(std::move(texture), target);
        }
    }

    template <typename ... T>
    void constructFromTextures(T&& ... params) {
        auto paramTuple = std::make_tuple(static_cast<FBConstructionPair>(params)...);
        std::apply([&](auto ...x){ (addBuffer<std::tuple_element<1, decltype(x)>::type::element_type>(std::move(x)), ...); }, paramTuple);
        
        assemble();
    }

    template <typename ... T>
    Framebuffer(T&& ... params) {
        glGenFramebuffers(1, &id);

        constructFromTextures(std::forward<T>(params)...);
    }

    void bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void bindDraw() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); }
    void bindRead() { glBindFramebuffer(GL_READ_FRAMEBUFFER, id); }
    void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    auto guard() { return Guard{this}; }

    bool valid() const { return bValid; }

    std::string completeness() {
        auto g = guard();
        static std::map<GLenum, std::string> errToStr{
            ESTR(GL_FRAMEBUFFER_COMPLETE),
            ESTR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT),
            ESTR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT),
            ESTR(GL_FRAMEBUFFER_UNSUPPORTED)
        };
        auto pos = errToStr.find(glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return pos != std::end(errToStr) ? pos->second : "Unknown error.";
    }

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

template <std::ranges::range R>
auto collect_set(R&& r) {
    using value_type = std::ranges::range_value_t<decltype(r)>;
    std::set<value_type> s{};
    
    std::ranges::copy(r, std::inserter(s, s.end()));
    return s;
}

template <std::ranges::range ... R>
auto collect_set(R&& ...r) {
    auto tup = std::make_tuple(r...);
    using value_type = std::ranges::range_value_t<std::tuple_element<0, decltype(tup)>::value>;
    std::set<value_type> s{};
    
    std::apply([&s](auto ... x){ (std::ranges::copy(x, std::inserter(s, s.end())),...); }, tup);
    return s;
}

template <typename T, std::size_t N>
constexpr auto arrSize(const T (&arr)[N]) { return N; }

// Specialization that removes \0 from str literal
template <std::size_t N>
constexpr auto arrSize(const char (&arr)[N]) { return N-1; }

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

}

#endif // UTILS_H