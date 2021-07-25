cmake_minimum_required(VERSION 3.16.0)

project(BlobbySpheres)

# C++ Project settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PARALLEL_BUILDING "Enable building using multiple cores" ON)
if (PARALLEL_BUILDING)
    set(PARALLEL_BUILDING_COUNT 8 CACHE STRING "How many max cores to simultaneous utilize (number)")
    set(CMAKE_BUILD_PARALLEL_LEVEL, PARALLEL_BUILDING_COUNT)
endif()

# Output executable
add_executable(BlobbySpheres)

# Source files
add_subdirectory(src)

find_package(Git QUIET)

if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git not found, cannot init submodules.")
else()
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
endif()

# Add library subdirectory cmakelists.txt:
add_subdirectory(lib)