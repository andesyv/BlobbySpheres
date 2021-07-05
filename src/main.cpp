#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include <chrono> // Timers
#include <iostream>
#include <entt/entt.hpp> // https://github.com/skypjack/entt
#include <glm/glm.hpp>   // https://github.com/g-truc/glm

#include "shader.h"
#include "timer.h"


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

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
std::string enumToString(GLenum arg, T... params)
{
    constexpr auto n = sizeof...(T);
    return enumToString<n>(arg, std::array<ESPair, n>{params...});
}

void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    std::string sourceStr{enumToString(source,
        ESPair{GL_DEBUG_SOURCE_API, "GL_DEBUG_SOURCE_API"},
        ESPair{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "GL_DEBUG_SOURCE_WINDOW_SYSTEM"},
        ESPair{GL_DEBUG_SOURCE_SHADER_COMPILER, "GL_DEBUG_SOURCE_SHADER_COMPILER"},
        ESPair{GL_DEBUG_SOURCE_THIRD_PARTY, "GL_DEBUG_SOURCE_THIRD_PARTY"},
        ESPair{GL_DEBUG_SOURCE_APPLICATION, "GL_DEBUG_SOURCE_APPLICATION"},
        ESPair{GL_DEBUG_SOURCE_OTHER, "GL_DEBUG_SOURCE_OTHER"}
    )};
    std::string typeStr{enumToString(type,
        ESPair{GL_DEBUG_TYPE_ERROR, "GL_DEBUG_TYPE_ERROR"},
        ESPair{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"},
        ESPair{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"},
        ESPair{GL_DEBUG_TYPE_PORTABILITY, "GL_DEBUG_TYPE_PORTABILITY"},
        ESPair{GL_DEBUG_TYPE_PERFORMANCE, "GL_DEBUG_TYPE_PERFORMANCE"},
        ESPair{GL_DEBUG_TYPE_MARKER, "GL_DEBUG_TYPE_MARKER"},
        ESPair{GL_DEBUG_TYPE_PUSH_GROUP, "GL_DEBUG_TYPE_PUSH_GROUP"},
        ESPair{GL_DEBUG_TYPE_POP_GROUP, "GL_DEBUG_TYPE_POP_GROUP"},
        ESPair{GL_DEBUG_TYPE_OTHER, "GL_DEBUG_TYPE_OTHER"}
    )};
    std::string severityStr{enumToString(severity,
        ESPair{GL_DEBUG_SEVERITY_HIGH, "GL_DEBUG_SEVERITY_HIGH"},
        ESPair{GL_DEBUG_SEVERITY_MEDIUM, "GL_DEBUG_SEVERITY_MEDIUM"},
        ESPair{GL_DEBUG_SEVERITY_LOW, "GL_DEBUG_SEVERITY_LOW"},
        ESPair{GL_DEBUG_SEVERITY_NOTIFICATION, "GL_DEBUG_SEVERITY_NOTIFICATION"}
    )};
    
    std::cout << "GL_ERROR: (source: " << sourceStr << ", type: " << typeStr << ", severity: " << severityStr << ", message: " << message << std::endl;
}

// settings
constexpr unsigned int SCR_WIDTH = 800;
constexpr unsigned int SCR_HEIGHT = 600;

struct Transform {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct Mesh {
    unsigned int VAO;
    unsigned int vertexCount;
    bool bIndices : 1;
    unsigned int indexCount : 7;     

    Mesh(unsigned int _VAO = 0, unsigned int _vCount = 0)
        : VAO{_VAO}, vertexCount{_vCount}, bIndices{false}
    {}

    Mesh(unsigned int _VAO, unsigned int _vCount, unsigned int _iCount)
        : VAO{_VAO}, vertexCount{_vCount}, bIndices{true}, indexCount{_iCount}
    {}
};

struct Material {
    int shader;
};

int main()
{
    Timer appTimer{};

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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(&errorCallback, nullptr);






    // ================== Setup scene ====================================

    // Shader defaultShader{
    //     {{GL_VERTEX_SHADER, "src/default.vert"},
    //     {GL_FRAGMENT_SHADER, "src/default.frag"}}
    // };
    Shader defaultShader{{
        {GL_VERTEX_SHADER, "default.vert"},
        {GL_FRAGMENT_SHADER, "default.frag"}
    }};


    // Setup
    entt::registry EM{};

    auto entity = EM.create();
    auto &mat = EM.emplace<Material>(entity, defaultShader.get());
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        0.5f, 0.5f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 3, // first Triangle
        1, 2, 3  // second Triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    EM.emplace<Mesh>(entity, VAO, static_cast<unsigned int>(sizeof(vertices)), static_cast<unsigned int>(sizeof(indices)));

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);








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
            // (Structured bindings ftw!! ENTT is so cool)
            auto [mesh, material] = view.get<Mesh, Material>(entity);

            // draw our first triangle
            glUseProgram(material.shader);
            glBindVertexArray(mesh.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            if (mesh.bIndices)
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
            else
                glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        }
        glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}