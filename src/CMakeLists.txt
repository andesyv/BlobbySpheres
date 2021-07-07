configure_file(constants.h.in ${CMAKE_BINARY_DIR}/generated/constants.h)
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_BINARY_DIR}/generated/)

target_sources(VSCodeOpenGL PRIVATE
    main.cpp
    components.cpp
    utils.cpp
)