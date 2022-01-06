#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp> // https://github.com/skypjack/entt
#include <glm/glm.hpp>   // https://github.com/g-truc/glm

#include <iostream>
#include <array>
#include <cassert>
#include <random> // For random seeding
#include <ctime> // For random seeding

#include "shader.h"
#include "timer.h"
#include "components.h"
#include "utils.h"
#include "scene.h"
#include "settings.h"
#include "camera.h"

template <std::size_t I>
std::string enumToString(GLenum arg, const std::array<std::pair<GLenum, std::string>, I>& params)
{
    for (std::size_t i{0}; i < I; ++i)
        if (params[i].first == arg)
            return params[i].second;
    
    return "UNDEFINED";
}

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
        // FPS
        const auto fps = frameCount * 1000.f / elapsed;
        frameCount = 0;
        timer.reset();

        std::string title = std::format("BlobbySpheres, fps: {}", fps);

        // Profiler
        const auto profiles = Profiler::get().getAvgTimesReset();
        if (!profiles.empty()) {
            title.reserve(util::arrSize(", Profiling: ") + title.size() + profiles.size() * util::arrSize("{00.00ms},"));
            title += ", Profiling: ";
            for (const auto& p : profiles)
                title += std::format("{{{}ms}},", util::to_string_with_precision(p * 0.001, 2));
        }

        glfwSetWindowTitle(window, title.c_str());
    }
    ++frameCount;
}

using namespace util;
using namespace comp;

int main()
{
    Timer appTimer{};
    std::srand(std::time(nullptr));
    auto [SCR_SIZE, runningTime] = get_multiple<0, 3>(Settings::get().to_tuple());
 
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_SIZE.x, SCR_SIZE.y, "BlobbySpheres", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    // ---------------------------------------------------------------------------------------------
    static const auto persp = [](glm::ivec2 size) { return glm::perspective(30.f, static_cast<float>(size.x) / size.y, 0.1f, 100.f); };
    Camera::getGlobalCamera().setPMat(persp(SCR_SIZE));
    static auto framebuffer_size_callback = [](GLFWwindow *window, int w, int h) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        auto& size = Settings::get().SCR_SIZE;
        size.x = static_cast<unsigned int>(w);
        size.y = static_cast<unsigned int>(h);
        glViewport(0, 0, w, h);

        Camera::getGlobalCamera().setPMat(persp({w, h}));
    };

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowPosCallback(window, [](auto* window, int x, int y){
        auto& pos = Settings::get().SCR_POS;
        pos.x = static_cast<unsigned int>(x);
        pos.y = static_cast<unsigned int>(y);
    });

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

        std::cout << "GL_DEBUG: (source: " << sourceStr << ", type: " << typeStr << ", severity: " << severityStr << ", message: " << message << std::endl;
//        assert(severity != GL_DEBUG_SEVERITY_HIGH);
    };

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(errorCallback, nullptr);


    // Dear ImGui: Setup context
    // --------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");



    {
        // uncomment this call to draw in wireframe polygons.
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        auto scene = Scene{};

        // Static wrapper ptr to scene, to get around static functions
        static auto scenePtr = &scene;


        // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
        // ---------------------------------------------------------------------------------------------------------
        static bool bCameraUpdated = true;

        glfwSetKeyCallback(window, [](auto window, int key, int scancode, int action, int mods){
            if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE)
                    glfwSetWindowShouldClose(window, true);
                
                if (key == GLFW_KEY_F5)
                    scenePtr->reloadShaders();
            }
        });

        glfwSetScrollCallback(window, [](auto window, double y, double x){
            const auto zoomSpeed = 0.01;
            auto& zoom = Settings::get().zoom;
            zoom = std::clamp(zoom + x * zoomSpeed, 0.0, 1.0);
            bCameraUpdated = true;
        });

        glfwSetCursorPosCallback(window, [](auto window, double x, double y){
            auto [size, scr_pos, mousePos] = get_multiple<0, 1, 2>(Settings::get().to_tuple());
            /**
             * f(screen_pos_0) = -1
             * f(screen_pos_0 + screen_size) = 1
             * f = ax + b
             * a * screen_pos_0 + b = -1 => b = -1 - a * screen_pos_0
             * a * (screen_pos_0 + screen_size) + b = 1
             * a * (screen_pos_0 + screen_size) + (-1 - a * screen_pos_0) = 1
             * a * screen_size - 1 = 1
             * a * screen_size = 2 => a = 2 / screen_size
             * b = -1 - a * screen_pos_0 => b = -1 - screen_pos_0 * 2 / screen_size
             * (2 / screen_size) * x + (-1 - screen_pos_0 * 2 / screen_size)
             * x * 2 / screen_size -1 - screen_pos_0 * 2 / screen_size
             * (x - screen_pos_0) * 2 / screen_size - 1
             */
            mousePos = (glm::dvec2{x, y} - glm::dvec2{scr_pos}) * 2.0 / glm::dvec2{size} - 1.0;
            bCameraUpdated = true;
        });


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
            runningTime = appTimer.elapsed<std::chrono::milliseconds>() * 0.001f;
            
            glfwPollEvents();

            showFPS(window);

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::BeginMainMenuBar();

            // Input
            // -----
            if (bCameraUpdated) {
                Camera::getGlobalCamera().calcMVP();
                bCameraUpdated = false;
            }

            // Start profiler frame:
            Profiler::get().newFrame();

            // render
            // ------
            scene.render(deltaTime);

            // ImGui render UI:
            ImGui::EndMainMenuBar();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(window);
        }
    }


    // Dear ImGui: Cleanup
    // -------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}