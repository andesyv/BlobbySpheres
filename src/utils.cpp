#include "utils.h"

#include <glm/gtc/type_ptr.hpp>

namespace util {

// vec{1-4}
template <> void uniform<float>(unsigned int location, const float& value)
{
    glUniform1f(location, value);
}

template <> void uniform<glm::vec2>(unsigned int location, const glm::vec2& value)
{
    glUniform2fv(location, 1, glm::value_ptr(value));
}

template <> void uniform<glm::vec3>(unsigned int location, const glm::vec3& value)
{
    glUniform3fv(location, 1, glm::value_ptr(value));
}

template <> void uniform<glm::vec4>(unsigned int location, const glm::vec4& value)
{
    glUniform4fv(location, 1, glm::value_ptr(value));
}



// mat{2-4}
template <> void uniform<glm::mat2>(unsigned int location, const glm::mat2& value)
{
    glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

template <> void uniform<glm::mat3>(unsigned int location, const glm::mat3& value)
{
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

template <> void uniform<glm::mat4>(unsigned int location, const glm::mat4& value)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}





bool Framebuffer::assemble() {
    // Atleast one color and depth attachment is required by OpenGL
    if (textures.empty() || !depthTexture)
        return false;

    auto g = guard();

    // Find rendertargets (if they aren't specified):
    if (rendertargets.empty()) {
        rendertargets.reserve(textures.size());
        for (unsigned int i{0}; i < textures.size(); ++i)
        rendertargets.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    // Bind buffers:
    for (auto [tex, target] : zip(textures, rendertargets))
        glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, tex->id, 0);

    if (depthTexture)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->id, 0);

    if (stencilTexture)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTexture->id, 0);

    // Set draw buffers
    glDrawBuffers(rendertargets.size(), reinterpret_cast<unsigned int*>(rendertargets.data()));

    // Return status
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

std::string Framebuffer::completeness() {
    auto g = guard();
    switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE:
            return "Complete!";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        // case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        //     return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";
        default:
            return "Unknown error.";
    }
}

}