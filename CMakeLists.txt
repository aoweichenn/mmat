cmake_minimum_required(VERSION 3.24)

project(
    mmat
    VERSION 0.1.0
    DESCRIPTION "ncnn Mat layout, storage and reference counting"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(mmat_build_tests "Build mmat tests" ${PROJECT_IS_TOP_LEVEL})
option(MMAT_SANITIZE "Enable ASan+UBSan" OFF)
option(MMAT_BATCH "Enable the batch dimension (must match the consumer)" OFF)

if(NOT TARGET mmemory::mmemory)
    set(MMAT_MMEMORY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../mmemory" CACHE PATH "mmemory source directory")
    if(NOT EXISTS "${MMAT_MMEMORY_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "mmat requires the mmemory source tree; set MMAT_MMEMORY_DIR")
    endif()
    set(mmemory_build_tests OFF CACHE BOOL "" FORCE)
    add_subdirectory("${MMAT_MMEMORY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/mmemory")
endif()

set(MMAT_WARNINGS_COMMON
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wold-style-cast
    -Wcast-qual
    -Wdouble-promotion
    -Wformat=2
    -Wundef
    -Wpointer-arith
    -Wwrite-strings
    -Wnull-dereference
    -Wzero-as-null-pointer-constant
)
set(MMAT_WARNINGS_GCC
    -Wmissing-declarations
    -Wredundant-decls
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)
set(MMAT_WARNINGS
    ${MMAT_WARNINGS_COMMON}
    $<$<CXX_COMPILER_ID:GNU>:${MMAT_WARNINGS_GCC}>
)

# The Mat core is a behaviour-preserving port of ncnn's BSD-3-Clause Mat.
# Keep it clean under the upstream warning level instead of rewriting every
# legacy C-style cast and pointer offset in the vendored code. New code
# (tests, scripts) still uses the strict MMAT_WARNINGS set.
set(MMAT_WARNINGS_PORTED
    -Wall
    -Wextra
    -Wpedantic
    -Wundef
    -Wpointer-arith
    -Wwrite-strings
    -Wnull-dereference
)

add_library(mmat_sanitizer INTERFACE)
if(MMAT_SANITIZE)
    target_compile_options(mmat_sanitizer INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS)
    target_link_options(mmat_sanitizer INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/mmat/config.hpp
    @ONLY)

add_library(mmat_objs OBJECT src/mat.cpp)
target_include_directories(mmat_objs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
        $<TARGET_PROPERTY:mmemory::mmemory,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:mplatform::mplatform,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_options(mmat_objs PRIVATE ${MMAT_WARNINGS_PORTED})
target_link_libraries(mmat_objs PRIVATE mmat_sanitizer)

add_library(mmat STATIC)
# Self-contained archive: Mat + mmemory + mplatform objects.
target_link_libraries(mmat PUBLIC
    $<BUILD_INTERFACE:mmat_objs>
    $<BUILD_INTERFACE:mmemory_objs>
    $<BUILD_INTERFACE:mplatform_objs>)
add_library(mmat::mmat ALIAS mmat)

target_include_directories(mmat
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(mmat INTERFACE $<BUILD_INTERFACE:mmat_sanitizer>)

if(PROJECT_IS_TOP_LEVEL)
    include(CMakePackageConfigHelpers)

    install(TARGETS mmat EXPORT mmatTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/mmat/config.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mmat)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../mmemory/include/mmemory
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mmemory/include/mmemory/config.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mmemory)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../mplatform/include/mplatform
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mmemory/mplatform/include/mplatform/config.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mplatform)
    install(EXPORT mmatTargets
        NAMESPACE mmat::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mmat)

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mmatConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/mmatConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mmat)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/mmatConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mmatConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/mmatConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mmat)
endif()

if(mmat_build_tests AND BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
