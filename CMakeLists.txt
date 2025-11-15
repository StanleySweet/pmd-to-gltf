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

# Test data generator
add_executable(generate_test_data tests/generate_test_data.c pmd_writer.c)
target_include_directories(generate_test_data PRIVATE .)
if(NOT WIN32)
    target_link_libraries(generate_test_data PRIVATE m)
endif()

# Unit tests
add_executable(test_filesystem tests/test_filesystem.c filesystem.c)
target_include_directories(test_filesystem PRIVATE .)
if(NOT WIN32)
    target_link_libraries(test_filesystem PRIVATE m)
endif()

add_executable(test_animation tests/test_animation.c)
target_include_directories(test_animation PRIVATE .)

add_executable(test_types tests/test_types.c)
target_include_directories(test_types PRIVATE .)
if(NOT WIN32)
    target_link_libraries(test_types PRIVATE m)
endif()

add_executable(test_pmd_cubes tests/test_pmd_cubes.c pmd_parser.c psa_parser.c)
target_include_directories(test_pmd_cubes PRIVATE .)
if(NOT WIN32)
    target_link_libraries(test_pmd_cubes PRIVATE m)
endif()

add_executable(test_gltf_output tests/test_gltf_output.c)
target_include_directories(test_gltf_output PRIVATE .)

add_executable(test_horse_model tests/test_horse_model.c pmd_parser.c psa_parser.c)
target_include_directories(test_horse_model PRIVATE .)
if(NOT WIN32)
    target_link_libraries(test_horse_model PRIVATE m)
endif()

add_executable(test_gltf_roundtrip tests/test_gltf_roundtrip.c pmd_parser.c)
target_include_directories(test_gltf_roundtrip PRIVATE .)
if(NOT WIN32)
    target_link_libraries(test_gltf_roundtrip PRIVATE m)
endif()

# Register unit tests
add_test(NAME unit_filesystem COMMAND test_filesystem)
add_test(NAME unit_animation COMMAND test_animation)
add_test(NAME unit_types COMMAND test_types)

# PMD cube integration tests - these require test data to be generated first
# Generate test data before running tests
add_test(NAME generate_test_data COMMAND generate_test_data)
set_tests_properties(generate_test_data PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME integration_pmd_cubes COMMAND test_pmd_cubes)
set_tests_properties(integration_pmd_cubes PROPERTIES
    DEPENDS generate_test_data
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Integration tests - Convert test cubes to glTF
add_test(
    NAME integration_cube_nobones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_nobones tests/output/cube_nobones.gltf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_nobones PROPERTIES
    DEPENDS generate_test_data
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_4bones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_4bones tests/output/cube_4bones.gltf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_4bones PROPERTIES
    DEPENDS generate_test_data
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_5bones
    COMMAND $<TARGET_FILE:converter> tests/data/cube_5bones tests/output/cube_5bones.gltf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_5bones PROPERTIES
    DEPENDS generate_test_data
    FIXTURES_SETUP gltf_outputs
)

add_test(
    NAME integration_cube_2bones_2props
    COMMAND $<TARGET_FILE:converter> tests/data/cube_2bones_2props tests/output/cube_2bones_2props.gltf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration_cube_2bones_2props PROPERTIES
    DEPENDS generate_test_data
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/input/horse.pmd")
    # Horse model validation tests
    add_test(NAME integration_horse_model COMMAND test_horse_model)
    set_tests_properties(integration_horse_model PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    add_test(
        NAME integration_converter_test
        COMMAND $<TARGET_FILE:converter> input/horse output/cmake_test.gltf Horse
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(integration_converter_test PROPERTIES
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