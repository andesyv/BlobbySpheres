configure_file(utils.h.in ${CMAKE_BINARY_DIR}/generated/utils.h)
target_include_directories(VSCodeOpenGL PRIVATE ${CMAKE_BINARY_DIR}/generated/)

target_sources(VSCodeOpenGL PRIVATE
    main.cpp
)