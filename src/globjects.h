#ifndef GLOBJECTS_H
#define GLOBJECTS_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <array>
#include <algorithm>
#include <vector>
#include <set>
#include <ranges>

#include "utils.h"

namespace globjects {

    template<typename T>
    concept Bindable = requires(T t) {
        t.bind();
        t.unbind();
    };

    template<typename T>
    concept BindableIndexed = requires(T t) {
        t.bind(0);
        t.unbind(0);
    };

// Util guard class
    template<typename C, typename P = void> requires BindableIndexed<C> || Bindable<C>
    class [[nodiscard]] Guard {
    };

    template<Bindable T>
    class [[nodiscard]] Guard<T> {
    protected:
        T *mPtr;

    public:
        explicit Guard(T *ptr)
                : mPtr{ptr} {
            if (mPtr)
                mPtr->bind();
        }

        Guard(const Guard&) = delete;
        Guard(Guard&& rhs)  noexcept : mPtr{rhs.mPtr} {
            rhs.mPtr = nullptr;
        };
        Guard& operator=(const Guard&) = delete;
        Guard& operator=(Guard&& rhs) noexcept {
            mPtr = rhs.mPtr;
            rhs.mPtr = nullptr;
            return *this;
        };

        ~Guard() {
            if (mPtr)
                mPtr->unbind();
        }
    };

    template<BindableIndexed T, typename IndexT>
    class [[nodiscard]] Guard<T, IndexT> {
    protected:
        T *mPtr;
        const IndexT mBinding = 0;

    public:
        explicit Guard(T *ptr, IndexT binding = 0)
                : mPtr{ptr}, mBinding{binding} {
            if (mPtr)
                mPtr->bind(binding);
        }

        Guard(const Guard&) = delete;
        Guard(Guard&& rhs)  noexcept : mPtr{rhs.mPtr} {
            rhs.mPtr = nullptr;
        };
        Guard& operator=(const Guard&) = delete;
        Guard& operator=(Guard&& rhs) noexcept {
            mPtr = rhs.mPtr;
            rhs.mPtr = nullptr;
            return *this;
        };

        ~Guard() {
            if (mPtr)
                mPtr->unbind();
        }
    };

    template<typename T>
    [[nodiscard]] auto make_guard(T *p) { return Guard<T>{p}; }

    template<typename T>
    [[nodiscard]] auto make_guard(T *p, unsigned int binding) { return Guard<T, unsigned int>{p, binding}; }

    template<GLenum BufferType>
    class Buffer {
    private:
        std::size_t bufferSize{};
    public:
        unsigned int id{};
        const GLenum type = BufferType;

        void bufferData(std::size_t byteSize, GLenum usage = GL_STATIC_DRAW) {
            [[maybe_unused]] auto g = guard();
            bufferSize = byteSize;
            glBufferData(BufferType, static_cast<GLsizeiptr>(byteSize), nullptr, usage);
        }

        template<typename T>
        void bufferData(const std::vector<T> &data, GLenum usage = GL_STATIC_DRAW) {
            auto g = guard();
            bufferSize = sizeof(T) * data.size();
            glBufferData(BufferType, bufferSize, data.data(), usage);
        }

        template<typename T>
        void updateBuffer(const std::vector<T> &data, GLintptr offset = 0) {
            auto g = guard();
            assert(bufferSize == sizeof(T) * data.size());
            glBufferSubData(BufferType, offset, sizeof(T) * data.size(), data.data());
        }

        Buffer() {
            glGenBuffers(1, &id);
        }

        explicit Buffer(std::size_t byteSize, GLenum usage = GL_STATIC_DRAW) {
            glGenBuffers(1, &id);
            bufferData(byteSize, usage);
        }

        template<typename T>
        explicit Buffer(const std::vector<T> &params, GLenum usage = GL_STATIC_DRAW) {
            glGenBuffers(1, &id);
            bufferData(params, usage);
        }

        void bindBase(unsigned int binding = 0) {
            glBindBufferBase(BufferType, binding, id);
        }

        void bind() {
            glBindBuffer(BufferType, id);
        }

        void bind(unsigned int binding) { bindBase(binding); }

        void unbind(unsigned int = 0) {
            glBindBuffer(BufferType, 0);
        }

        [[nodiscard]] auto guard() { return make_guard(this); }

        [[nodiscard]] auto guard(unsigned int binding) { return make_guard(this, binding); }

        [[nodiscard]] auto size() const { return bufferSize; }


        // Same as bindBase, but also let's you specify range of bound buffer (glBindBufferRange)
        void bindRange(GLsizeiptr size, unsigned int binding = 0, GLintptr offset = 0) {
            glBindBufferRange(BufferType, binding, id, offset, size);
        }

        ~Buffer() {
            glDeleteBuffers(1, &id);
        }
    };

    template<GLenum TextureType, typename DimType>
    class TextureBase {
    public:
        unsigned int id{};
        const GLenum type = TextureType;
        DimType texSize;
        GLenum texInternalFormat{};
        // Format of data transfered to texture for clear and init purposes.
        // Can usually be determined from internalformat.
        GLenum texDataFormat{};

        TextureBase() {
            glGenTextures(1, &id);
        }

        virtual void data(GLint level, GLenum internalformat, DimType size, GLint border, GLenum format, GLenum type,
                          const void *data) = 0;

        virtual void init(DimType size, GLenum internalformat, GLenum format) = 0;

        void clear() {
            bind();
            data(0, texInternalFormat, texSize, 0, texDataFormat, GL_UNSIGNED_BYTE, nullptr);
        };

        void bind(unsigned int textureUnit = 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(TextureType, id);
        }

        void unbind(unsigned int textureUnit = 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(TextureType, 0);
        }

        [[nodiscard]] auto guard(unsigned int textureUnit = 0) { return make_guard(this, textureUnit); }

        ~TextureBase() {
            glDeleteTextures(1, &id);
        }
    };

    template<GLenum TextureType>
    class Texture : public TextureBase<TextureType, void> {
    };

// template <>
// class Texture<GL_TEXTURE_1D> : public TextureBase<GL_TEXTURE_1D, glm::vec1> {
// public:
//     void textureData(GLint level, GLint internalformat, float size, GLint border, GLenum format, GLenum type, const void * data) {
//         glTexImage1D(GL_TEXTURE_1D, level, internalformat, size, border, format, type, data);
//     }

//     void init() {

//     }
// };

    template<>
    class Texture<GL_TEXTURE_2D> : public TextureBase<GL_TEXTURE_2D, glm::ivec2> {
    public:
        void data(GLint level, GLenum internalformat, glm::ivec2 size, GLint border, GLenum format, GLenum type,
                  const void *data) final {
            glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(internalformat), size.x, size.y, border, format, type, data);
            texSize = size;
            texInternalFormat = internalformat;
            texDataFormat = format;
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

        explicit Texture(glm::ivec2 size, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA)
                : TextureBase<GL_TEXTURE_2D, glm::ivec2>{} {
            init(size, internalformat, format);
        }
    };

    using Tex2D = Texture<GL_TEXTURE_2D>;

    template<>
    class Texture<GL_TEXTURE_3D> : public TextureBase<GL_TEXTURE_3D, glm::ivec3> {
    public:
        void data(GLint level, GLenum internalformat, glm::ivec3 size, GLint border, GLenum format, GLenum type,
                  const void *data) final {
            glTexImage3D(GL_TEXTURE_3D, level, static_cast<GLint>(internalformat), size.x, size.y, size.z, border, format, type, data);
            texSize = size;
            texInternalFormat = internalformat;
            texDataFormat = format;
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

        explicit Texture(glm::ivec3 size, GLenum internalformat = GL_RGBA16F, GLenum format = GL_RGBA)
                : TextureBase<GL_TEXTURE_3D, glm::ivec3>{} {
            init(size, internalformat, format);
        }
    };

    using Tex3D = Texture<GL_TEXTURE_3D>;


    class RenderBuffer {
    public:
        unsigned int id;

        RenderBuffer() {
            glGenRenderbuffers(1, &id);
        }

        void bind() {
            glBindBuffer(GL_RENDERBUFFER, id);
        }

        void unbind() {
            glBindBuffer(GL_RENDERBUFFER, 0);
        }

        [[nodiscard]] auto guard() { return make_guard(this); }

        void init(glm::ivec2 size = glm::ivec2{}, GLenum internalformat = GL_RGBA16F) {
            auto g = guard();

            glRenderbufferStorage(GL_RENDERBUFFER, internalformat, size.x, size.y);
        }

        ~RenderBuffer() {
            glDeleteRenderbuffers(1, &id);
        }
    };

    class VertexArray {
    private:
        bool bInit = false;

    public:
        unsigned int id;
        std::unique_ptr<Buffer<GL_ARRAY_BUFFER>> vertexBuffer;
        std::unique_ptr<Buffer<GL_ELEMENT_ARRAY_BUFFER>> indexBuffer;

        template<typename T>
        VertexArray(const std::vector<T> &vertices, GLenum usage = GL_STATIC_DRAW) : bInit{true} {
            glGenVertexArrays(1, &id);
            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
            glBindVertexArray(id);

            vertexBuffer = std::make_unique<Buffer<GL_ARRAY_BUFFER>>(vertices, usage);
            vertexBuffer->bind();
        }

        template<typename T, std::size_t I>
        VertexArray(T (&&vertices)[I]) : VertexArray{std::vector{vertices}} {}

        template<typename T, std::size_t I>
        VertexArray(const std::array<T, I> &vertices) : VertexArray{std::vector{vertices}} {}

        template<typename T, typename U>
        VertexArray(const std::vector<T> &vertices, const std::vector<U> &indices, GLenum usage = GL_STATIC_DRAW)
                : bInit{true} {
            glGenVertexArrays(1, &id);

            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
            glBindVertexArray(id);

            vertexBuffer = std::make_unique<Buffer<GL_ARRAY_BUFFER>>(vertices, usage);
            vertexBuffer->bind();

            indexBuffer = std::make_unique<Buffer<GL_ELEMENT_ARRAY_BUFFER>>(indices, usage);
            indexBuffer->bind();
        }

        void vertexAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride = 0,
                             const void *pointer = nullptr) {
            glVertexAttribPointer(index, size, type, normalized, stride, pointer);
            glEnableVertexAttribArray(index);
        }

        void bind() {
            glBindVertexArray(id);
        }

        void unbind() {
            glBindVertexArray(0);
        }

        [[nodiscard]] auto guard() { return make_guard(this); }

        bool hasIndices() const { return static_cast<bool>(indexBuffer); }

        ~VertexArray() {
            if (bInit)
                glDeleteVertexArrays(1, &id);
        }
    };

    template<class T>
    concept TextureType = requires(T t) {
        { Texture{t}} -> std::same_as<T>;
    };

    template<typename T>
    concept FBBufferType = TextureType<T> || std::same_as<T, RenderBuffer>;

    template<typename T>
    using FBConstructionPair = std::pair<GLenum, std::shared_ptr<T>>;

    template<FBBufferType ... ColorBufferTypes>
    class GenericFramebuffer {
    private:
        bool bValid = false;

        template<FBBufferType T>
        void bindTexture(const std::pair<std::shared_ptr<T>, GLenum> &arg) {
            auto[tex, target] = arg;
            if constexpr (std::is_same_v<T, Tex2D>)
                glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, tex->id, 0);
            else if constexpr (std::is_same_v<T, Tex3D>)
                glFramebufferTexture3D(GL_FRAMEBUFFER, target, GL_TEXTURE_3D, tex->id, 0, 0);
        }

    public:
        template<FBBufferType T>
        using BufferList = std::vector<std::pair<std::shared_ptr<T>, GLenum>>;

        unsigned int id;
        std::tuple<BufferList<ColorBufferTypes>...> colorBuffers;
        std::shared_ptr<Texture<GL_TEXTURE_2D>> depthTexture;
        std::shared_ptr<Texture<GL_TEXTURE_2D>> stencilTexture;

        auto getColorBufferCount() const {
            return std::apply([](auto ...list) { return (list.size() + ...); }, colorBuffers);
        }

        std::set<GLenum> getRendertargets() const {
            return std::apply([](auto ...list) {
                return collect_set((list | std::views::transform([](const auto &v) { return v.second; }))...);
            }, colorBuffers);
        }

        template<FBBufferType T>
        auto &addColorBuffer(std::shared_ptr<T> &&texture, GLenum target = GL_COLOR_ATTACHMENT0) {
            auto &bufferList = std::get<BufferList<T>>(colorBuffers);
            return bufferList.emplace_back(std::move(texture), target);
        }

        void addDepthTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>> &&texture) {
            depthTexture = std::move(texture);
        }

        void addStencilTexture(std::shared_ptr<Texture<GL_TEXTURE_2D>> &&texture) {
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
                std::apply([&](auto &...list) {
                    ([&](auto &l) {
                        for (auto &v: l)
                            v.second = GL_COLOR_ATTACHMENT0 + (++i - 1);
                    }(list), ...);
                }, colorBuffers);
                targets = getRendertargets();
            }

            // Bind buffers:
            std::apply([&](const auto &...list) {
                ([&](const auto &l) {
                    for (const auto &v: l)
                        bindTexture(v);
                }(list), ...);
            }, colorBuffers);

            if (depthTexture)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->id, 0);

            if (stencilTexture)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTexture->id, 0);

            // Set draw buffers
            const std::vector<GLenum> targetList{targets.begin(), targets.end()};
            glDrawBuffers(targetList.size(), reinterpret_cast<unsigned int const *>(targetList.data()));

            // Return status
            bValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
            return bValid;
        }

        GenericFramebuffer() {
            glGenFramebuffers(1, &id);
        }

        template<FBBufferType T>
        void addBuffer(FBConstructionPair<T> &&param) {
            auto[target, texture] = param;
            addColorBuffer<T>(std::move(texture), target);
        }

        template<>
        void addBuffer(FBConstructionPair<Tex2D> &&param) {
            auto[target, texture] = param;

            if (target == GL_DEPTH_ATTACHMENT)
                addDepthTexture(std::move(texture));
            else if (target == GL_STENCIL_ATTACHMENT)
                addStencilTexture(std::move(texture));
                // Enable this part only if we have a Tex2D buffer in the available buffers:
            else if constexpr ((std::is_same_v<Tex2D, ColorBufferTypes> || ...)) { // https://stackoverflow.com/questions/2118541/check-if-parameter-pack-contains-a-type
                addColorBuffer<Tex2D>(std::move(texture), target);
            }
        }

        template<typename ... T>
        void constructFromTextures(T &&... params) {
            auto paramTuple = std::make_tuple(static_cast<FBConstructionPair>(params)...);
            std::apply([&](auto ...x) {
                (addBuffer<std::tuple_element<1, decltype(x)>::type::element_type>(std::move(x)), ...);
            }, paramTuple);

            assemble();
        }

        template<typename ... T>
        GenericFramebuffer(T &&... params) {
            glGenFramebuffers(1, &id);

            constructFromTextures(std::forward<T>(params)...);
        }

        void bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }

        void bindDraw() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); }

        void bindRead() { glBindFramebuffer(GL_READ_FRAMEBUFFER, id); }

        void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

        [[nodiscard]] auto guard() { return make_guard(this); }

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

        ~GenericFramebuffer() {
            glDeleteFramebuffers(1, &id);
        }
    };

    using Framebuffer = GenericFramebuffer<Tex2D>;

}

#endif // GLOBJECTS_H