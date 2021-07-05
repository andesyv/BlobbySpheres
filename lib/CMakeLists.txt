# GLAD
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
target_sources(VSCodeOpenGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)

# GLFW:
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(glfw)
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
target_link_libraries(VSCodeOpenGL PRIVATE glfw)

# OpenGL
if (WIN32)
    target_link_libraries(VSCodeOpenGL PRIVATE opengl32.lib)
endif()

# GLM:
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# ENTT
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/entt/src)
