cmake_minimum_required(VERSION 3.22)

project(raytracer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------
# User options
# ------------------------
option(ENABLE_CUDA "Build with CUDA support" OFF)

# ------------------------
# Dependencies
# ------------------------
find_package(Boost CONFIG COMPONENTS filesystem iostreams system REQUIRED)
find_package(PNG REQUIRED)
find_package(MPI REQUIRED)

if (ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    enable_language(CUDA)
    set(VSNRAY_ENABLE_CUDA ON)
    message(STATUS "CUDA support enabled: ${CUDAToolkit_VERSION}")
else()
    set(VSNRAY_ENABLE_CUDA OFF)
    message(STATUS "CUDA disabled")
endif()

# ------------------------
# Executable
# ------------------------
add_executable(raytracer)

set(COMMON_SOURCES
    3rdparty/CmdLine/src/CmdLine.cpp
    3rdparty/CmdLine/src/CmdLineUtil.cpp
    common/file_base.cpp
    common/image.cpp
    common/image_base.cpp
    common/pixel_format.cpp
    common/png_image.cpp
    common/sg.cpp
)

if (VSNRAY_ENABLE_CUDA)
    target_sources(raytracer PRIVATE
        ${COMMON_SOURCES}
        main.cu
    )
    set_property(TARGET raytracer PROPERTY CUDA_SEPARABLE_COMPILATION ON)
else()
    target_sources(raytracer PRIVATE
        ${COMMON_SOURCES}
        main.cpp
    )
endif()

# ------------------------
# Includes
# ------------------------
target_include_directories(raytracer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/3rdparty/CmdLine/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/3rdparty/visionaray/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/3rdparty/GL>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    ${Boost_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_DIRS}
)

# ------------------------
# Libraries
# ------------------------
target_link_libraries(raytracer PRIVATE
    Boost::disable_autolinking
    Boost::filesystem
    Boost::iostreams
    PNG::PNG
    ${MPI_CXX_LIBRARIES}
)

# ------------------------
# Definitions
# ------------------------
target_compile_definitions(raytracer PRIVATE GLEW_NO_GLU)

