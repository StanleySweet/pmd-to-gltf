cmake_minimum_required(VERSION 3.16)

project(pmd-to-gltf
    VERSION 1.0.0
    DESCRIPTION "PMD to glTF Converter"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2)
    endif()
endif()

# Source files
set(SOURCES
    main.c
    pmd_parser.c
    psa_parser.c
    gltf_exporter.c
    skeleton.c
    filesystem.c
)

set(HEADERS
    pmd_psa_types.h
    skeleton.h
    filesystem.h
)

# Create executable
add_executable(converter ${SOURCES} ${HEADERS})

# Link libraries
if(NOT WIN32)
    # Math library needed on Unix-like systems
    target_link_libraries(converter PRIVATE m)
endif()

# Platform-specific configuration
if(UNIX)
    # Enable POSIX extensions on Unix
    target_compile_definitions(converter PRIVATE _GNU_SOURCE)
endif()

# Installation
install(TARGETS converter
    RUNTIME DESTINATION bin
)

# Testing
enable_testing()

# Add a simple test if input files exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/input/horse.pmd")
    add_test(
        NAME converter_test
        COMMAND $<TARGET_FILE:converter> input/horse output/cmake_test.gltf Horse
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(converter_test PROPERTIES
        PASS_REGULAR_EXPRESSION "Done! Exported [0-9]+ animation"
    )
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "pmd-to-gltf")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "StanleySweet")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "PMD to glTF Converter")
    set(CPACK_NSIS_PACKAGE_NAME "PMD to glTF Converter")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "StanleySweet")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
endif()

include(CPack)

# Print build information
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==============================")
message(STATUS "")