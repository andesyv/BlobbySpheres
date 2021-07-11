configure_file(constants.h.in ${CMAKE_BINARY_DIR}/generated/constants.h)
target_include_directories(BlobbySpheres PRIVATE ${CMAKE_BINARY_DIR}/generated/)

target_sources(BlobbySpheres PRIVATE
    main.cpp
    components.cpp
    utils.cpp
    shader.cpp
    scene.cpp
)