#include "utils.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

namespace util {

// uivec1
template <> void uniform<glm::uint>(unsigned int location, const glm::uint& value)
{
    glUniform1ui(location, value);
}

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


glm::vec3 randomDiskPoint(glm::vec3 n, float r) {
    auto q = glm::vec3{1.f, 0.f, 0.f};
    if (glm::dot(n, glm::vec3{0.f, 1.f, 0.f}) < glm::dot(n, q))
        q = glm::vec3{0.f, 1.f, 0.f};
    
    auto u = glm::normalize(glm::cross(n, q));
    auto v = glm::normalize(glm::cross(n, u));

    auto deg = glm::linearRand(0.f, 2.f * glm::pi<float>());
    return r * u * std::sin(deg) + r * v * std::cos(deg);
}

}