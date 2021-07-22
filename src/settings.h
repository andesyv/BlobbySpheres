#ifndef SETTINGS_H
#define SETTINGS_H

#include <glm/fwd.hpp>
#include <tuple>

template <std::size_t ... I, typename ... T>
auto get_multiple(const std::tuple<T...>& tuple) {
    return std::forward_as_tuple(std::get<I>(tuple)...);
}

class Settings {
public:
    unsigned int SCR_WIDTH = 800;
    unsigned int SCR_HEIGHT = 600;
    glm::dvec2 mousePos;
    float runningTime = 0.f;
    double zoom = 0.5;

    static Settings& get() {
        static Settings instance{};
        return instance;
    }

    auto to_tuple() {
        auto& [a, b, c, d, e] = *this;
        return std::forward_as_tuple(a, b, c, d, e);
    }
};

#endif // SETTINGS_H