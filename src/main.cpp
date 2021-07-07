#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp> // https://github.com/skypjack/entt
#include <glm/glm.hpp>   // https://github.com/g-truc/glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

#include <array>
#include <chrono> // Timers
#include <iostream>
#include <cassert>
#include <random>
#include <ctime> // For random seeding
#include <format>

#include "shader.h"
#include "timer.h"
#include "components.h"
#include "utils.h"

#define ESTR(x) ESPair{x, #x}

template <std::size_t I>
std::string enumToString(GLenum arg, const std::array<std::pair<GLenum, std::string>, I>& params)
{
    for (std::size_t i{0}; i < I; ++i)
        if (params[i].first == arg)
            return params[i].second;
    
    return "UNDEFINED";
}

typedef std::pair<GLenum, std::string> ESPair;

template <typename... T>
std::string enumToString(GLenum arg, T... params) {
    constexpr auto n = sizeof...(T);
    return enumToString<n>(arg, std::array<ESPair, n>{params...});
}

void showFPS(GLFWwindow* window)
{
    static Timer timer{};
    static unsigned int frameCount{0};
    auto elapsed = timer.elapsed<std::chrono::milliseconds>();
    if (elapsed >= 1000)
    {
        const auto fps = frameCount * 1000.f / elapsed;
        std::string title{"VSCodeOpenGL, fps: " + std::to_string(fps)};
        glfwSetWindowTitle(window, title.c_str());
        frameCount = 0;
        timer.reset();
    }
    ++frameCount;
}

using namespace util;
using namespace comp;

// settings
static unsigned int SCR_WIDTH = 800;
static unsigned int SCR_HEIGHT = 600;
static glm::dvec2 mousePos;
static float runningTime = 0.f;

int main()
{
    Timer appTimer{};
    std::srand(std::time(nullptr));

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VSCodeOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    // ---------------------------------------------------------------------------------------------
    static auto framebuffer_size_callback = [](GLFWwindow *window, int width, int height) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        SCR_WIDTH = static_cast<unsigned int>(width);
        SCR_HEIGHT = static_cast<unsigned int>(height);
        glViewport(0, 0, width, height);
    };

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glm::ivec2 version;
    glGetIntegerv(GL_MAJOR_VERSION, &version.x);
    glGetIntegerv(GL_MINOR_VERSION, &version.y);
    std::cout << "Running OpenGL version: " << version.x << "." << version.y << std::endl;

    static auto errorCallback = [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
        static const std::map<GLenum, std::string> SOURCES{
            ESTR(GL_DEBUG_SOURCE_API),
            ESTR(GL_DEBUG_SOURCE_WINDOW_SYSTEM),
            ESTR(GL_DEBUG_SOURCE_SHADER_COMPILER),
            ESTR(GL_DEBUG_SOURCE_THIRD_PARTY),
            ESTR(GL_DEBUG_SOURCE_APPLICATION),
            ESTR(GL_DEBUG_SOURCE_OTHER)
        };

        static const std::map<GLenum, std::string> TYPES{
            ESTR(GL_DEBUG_TYPE_ERROR),
            ESTR(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR),
            ESTR(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR),
            ESTR(GL_DEBUG_TYPE_PORTABILITY),
            ESTR(GL_DEBUG_TYPE_PERFORMANCE),
            ESTR(GL_DEBUG_TYPE_MARKER),
            ESTR(GL_DEBUG_TYPE_PUSH_GROUP),
            ESTR(GL_DEBUG_TYPE_POP_GROUP),
            ESTR(GL_DEBUG_TYPE_OTHER)
        };

        static const std::map<GLenum, std::string> SEVERITIES{
            ESTR(GL_DEBUG_SEVERITY_HIGH),
            ESTR(GL_DEBUG_SEVERITY_MEDIUM),
            ESTR(GL_DEBUG_SEVERITY_LOW),
            ESTR(GL_DEBUG_SEVERITY_NOTIFICATION)
        };
        
        const auto [sourceStr, typeStr, severityStr] = std::tie(SOURCES.at(source), TYPES.at(type), SEVERITIES.at(severity));

        std::cout << "GL_ERROR: (source: " << sourceStr << ", type: " << typeStr << ", severity: " << severityStr << ", message: " << message << std::endl;
        assert(severity == GL_DEBUG_SEVERITY_NOTIFICATION);
    };

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(errorCallback, nullptr);



    {
        // ================== Setup scene ====================================
        constexpr auto SCENE_SIZE = 100u;

        Shader defaultShader{{
            {GL_VERTEX_SHADER, "default.vert"},
            {GL_FRAGMENT_SHADER, "default.frag"}
        }};

        Shader surfaceShader{{
            {GL_VERTEX_SHADER, "screen.vert"},
            {GL_FRAGMENT_SHADER, "sdf.frag"}
        }, {
            std::format("SCENE_SIZE {}u", SCENE_SIZE)
        }};


        // Setup
        entt::registry EM{};

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
        auto screenSpacedVAO = std::make_shared<VertexArray>(std::vector{
            -1.f, -1.f, 0.0f, // bottom left
            1.f, 1.f, 0.0f,   // top right
            1.f, -1.f, 0.0f,  // bottom right

            -1.f, -1.f, 0.0f, // bottom left
            -1.f, 1.f, 0.0f,   // top left
            1.f, 1.f, 0.0f   // top right
        });
        screenSpacedVAO->vertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);

        auto surfaceQuad = EM.create();
        auto &mat = EM.emplace<Material>(surfaceQuad, surfaceShader.get());
        EM.emplace<Mesh>(surfaceQuad, screenSpacedVAO, 6);


        std::vector<glm::vec4> scene;
        scene.reserve(SCENE_SIZE);
        for (unsigned int i = 0; i < SCENE_SIZE; ++i) {
            const auto pos = glm::linearRand(glm::vec3{-1.f}, glm::vec3{1.f}) * 0.3f;
            const auto radius = glm::linearRand(0.01f, 0.1f);
            scene.push_back(glm::vec4{pos, radius});
        }

        auto sceneBuffer = std::make_unique<Buffer<GL_SHADER_STORAGE_BUFFER>>(scene);
        sceneBuffer->bindBase(0);


        // uncomment this call to draw in wireframe polygons.
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        const auto pMat = glm::perspective(30.f, static_cast<float>(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 100.f);
        const auto calcMVP = [&](){
            constexpr auto cameraDist = 1.0f;

            const auto rot = 
            glm::angleAxis(static_cast<float>(mousePos.y), glm::vec3{1.f, 0.f, 0.f}) *
            glm::angleAxis(static_cast<float>(mousePos.x), glm::vec3{0.f, 1.f, 0.f});
            const auto vMat = glm::translate(glm::mat4{1.f}, glm::vec3{0.f, 0.f, -cameraDist}) * glm::mat4{glm::normalize(rot)};
            const auto MVP = pMat * vMat;
            return glm::inverse(MVP);
        };
        
        auto MVPInverse = calcMVP();




        // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
        // ---------------------------------------------------------------------------------------------------------
        const auto processInput = [&](GLFWwindow *window) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
            mousePos = (mousePos / glm::dvec2{SCR_WIDTH, SCR_HEIGHT}) * 2.0 - 1.0;
            MVPInverse = calcMVP();
        };


        std::cout << "Setup took " << appTimer.elapsed<std::chrono::milliseconds>() << "ms." << std::endl;
        appTimer.reset();
        
        Timer frameTimer{};

        // render loop
        // -----------
        while (!glfwWindowShouldClose(window))
        {
            // Find time since last frame
            const auto deltaTime = frameTimer.elapsed<std::chrono::milliseconds>() * 0.001f;
            frameTimer.reset();
            runningTime += deltaTime;

            showFPS(window);

            // input
            // -----
            processInput(window);

            // render
            // ------
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            auto view = EM.view<Mesh, Material>();
            for (const auto& entity : view) 
            {
                auto [mesh, material] = view.get<Mesh, Material>(entity);

                // draw our first triangle
                glUseProgram(material.shader);
                uniform(material, "MVPInverse", MVPInverse);
                uniform(material, "time", runningTime);
                mesh.draw();
            }
            glBindVertexArray(0); // no need to unbind it every time

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}