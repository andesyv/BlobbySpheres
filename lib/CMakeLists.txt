# GLAD
target_include_directories(ext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
target_sources(ext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)

# GLFW:
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(glfw)
target_include_directories(ext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
target_link_libraries(ext PRIVATE glfw)
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)

# OpenGL
if (WIN32)
    target_link_libraries(ext PRIVATE opengl32.lib)
endif()

# GLM:
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# ENTT
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/entt/src)

# Dear ImGui
target_include_directories(ext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_sources(ext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)