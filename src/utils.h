#ifndef UTILS_H
#define UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <ranges>
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

template <std::size_t N, typename T, T InitVal>
constexpr std::array<T, N> make_const_array() {
    std::array<T, N> arr;
    for (std::size_t i{0}; i < N; ++i)
        arr[i] = InitVal;
    return arr;
}

template <typename T>
auto gen_vec(std::size_t N, T initVal = {}) {
    std::vector<T> v;
    v.resize(N, initVal);
    return v;
}

}

#endif // UTILS_H