cmake_minimum_required(VERSION 3.16)

project(pmd-to-gltf
    VERSION 1.0.0
    DESCRIPTION "PMD to glTF Converter"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add cJSON library
add_library(cjson STATIC vendor/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC vendor/cJSON)
if(NOT WIN32)
    target_link_libraries(cjson PRIVATE m)
endif()

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
    src/main.c
    src/pmd_parser.c
    src/psa_parser.c
    src/gltf_exporter.c
    src/skeleton.c
    src/filesystem.c
    src/json_builder.c
)

set(HEADERS
    src/pmd_psa_types.h
    src/skeleton.h
    src/filesystem.h
    src/json_builder.h
)

# Create executable
    add_executable(test_writer tests/test_writer.c tests/pmd_writer.c)
    target_include_directories(test_writer PRIVATE src)
    if(NOT WIN32)
         target_link_libraries(test_writer PRIVATE m)
    endif()
add_executable(converter ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(converter PRIVATE cjson)
if(NOT WIN32)
    # Math library needed on Unix-like systems
    target_link_libraries(converter PRIVATE m)
endif()

# Include cJSON headers
target_include_directories(converter PRIVATE vendor/cJSON)

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



# Unit tests
add_executable(test_filesystem tests/test_filesystem.c src/filesystem.c)
target_include_directories(test_filesystem PRIVATE src)
if(NOT WIN32)
    target_link_libraries(test_filesystem PRIVATE m)
endif()

add_executable(test_animation tests/test_animation.c)
target_include_directories(test_animation PRIVATE src)

add_executable(test_types tests/test_types.c)
target_include_directories(test_types PRIVATE src)
if(NOT WIN32)
    target_link_libraries(test_types PRIVATE m)
endif()

add_executable(test_pmd_cubes tests/test_pmd_cubes.c src/pmd_parser.c src/psa_parser.c)
target_include_directories(test_pmd_cubes PRIVATE src)
if(NOT WIN32)
    target_link_libraries(test_pmd_cubes PRIVATE m)
endif()

add_executable(test_gltf_output tests/test_gltf_output.c tests/pmd_writer.c src/gltf_exporter.c src/pmd_parser.c src/json_builder.c src/psa_parser.c)
target_include_directories(test_gltf_output PRIVATE src vendor/cJSON)
target_link_libraries(test_gltf_output PRIVATE cjson)


add_executable(test_gltf_roundtrip tests/test_gltf_roundtrip.c tests/pmd_writer.c src/pmd_parser.c src/gltf_exporter.c src/json_builder.c src/psa_parser.c)
target_include_directories(test_gltf_roundtrip PRIVATE src vendor/cJSON)
target_link_libraries(test_gltf_roundtrip PRIVATE cjson)
if(NOT WIN32)
    target_link_libraries(test_gltf_roundtrip PRIVATE m)
endif()

# Register unit tests
add_test(NAME unit_filesystem COMMAND test_filesystem)
add_test(NAME unit_animation COMMAND test_animation)
add_test(NAME unit_types COMMAND test_types)



add_test(NAME integration_pmd_cubes COMMAND test_pmd_cubes)
set_tests_properties(integration_pmd_cubes PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Integration tests - Convert test cubes to glTF
add_test(
    NAME integration_cube_nobones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_nobones
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_nobones PROPERTIES
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_4bones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_4bones
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_4bones PROPERTIES
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_5bones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_5bones
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_5bones PROPERTIES
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_2bones_2props
    COMMAND $<TARGET_FILE:converter> tests/data/cube_2bones_2props
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_2bones_2props PROPERTIES
    FIXTURES_SETUP gltf_outputs
)

# Validate glTF output - runs after conversion tests
add_test(NAME validation_gltf_output COMMAND test_gltf_output)
set_tests_properties(validation_gltf_output PROPERTIES
    FIXTURES_REQUIRED gltf_outputs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Round-trip validation - verify glTF data matches original PMD
add_test(NAME validation_gltf_roundtrip COMMAND test_gltf_roundtrip)
set_tests_properties(validation_gltf_roundtrip PROPERTIES
    FIXTURES_REQUIRED gltf_outputs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Integration test - Add a simple test if input files exist

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