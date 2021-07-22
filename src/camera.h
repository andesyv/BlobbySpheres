#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "settings.h"

class Camera {
private:
    glm::mat4 pMat;
    glm::mat4 pMatInverse;
    glm::mat4 vMat;
    glm::mat4 MVP;
    glm::mat4 MVPInverse;

public:
    void calcMVP() {
        constexpr auto cameraDist = 2.0f;
        const auto& mousePos = Settings::get().mousePos;
        const auto& zoom = Settings::get().zoom;

        const auto rot = 
            glm::angleAxis(static_cast<float>(mousePos.y), glm::vec3{1.f, 0.f, 0.f}) *
            glm::angleAxis(static_cast<float>(mousePos.x), glm::vec3{0.f, 1.f, 0.f});
        vMat = glm::translate(glm::mat4{1.f}, glm::vec3{0.f, 0.f, std::lerp(-cameraDist, 0.f, static_cast<float>(zoom))}) * glm::mat4{glm::normalize(rot)};
        MVP = pMat * vMat;
        MVPInverse = glm::inverse(MVP);
    }

    void setPMat(const glm::mat4& mat) {
        pMat = mat;
        pMatInverse = glm::inverse(pMat);
        calcMVP();
    }

    void setVMat(const glm::mat4& mat) {
        vMat = mat;
        calcMVP();
    }

    auto getPMat() const { return pMat; }
    auto getPMatInverse() const { return pMatInverse; }
    auto getVMat() const { return vMat; }
    auto getMVP() const { return MVP; }
    auto getMVPInverse() const { return MVPInverse; }

    static Camera& getGlobalCamera() {
        static Camera instance{};
        return instance;
    }
};

#endif // CAMERA_H