#include "utils.h"

#include <glm/gtc/type_ptr.hpp>

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

}