cmake_minimum_required(VERSION 3.22)

project(raytracer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG COMPONENTS filesystem iostreams system REQUIRED)
find_package(PNG REQUIRED)
find_package(MPI REQUIRED)

find_package(CUDA)
if (CUDA_FOUND)
  enable_language(CUDA)
  set(VSNRAY_ENABLE_CUDA ON CACHE BOOL "Enable CUDA support")
endif()

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
else()
    target_sources(raytracer PRIVATE
        ${COMMON_SOURCES}
        main.cpp
    )
endif()

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

target_link_libraries(raytracer PRIVATE
    Boost::disable_autolinking
    Boost::filesystem
    Boost::iostreams
    PNG::PNG
    ${MPI_CXX_LIBRARIES}
)

if (VSNRAY_ENABLE_CUDA)
    target_link_libraries(raytracer PUBLIC visionaray visionaray_cuda)
    set_property(TARGET raytracer PROPERTY CUDA_SEPARABLE_COMPILATION ON)
endif()

target_compile_definitions(raytracer PRIVATE GLEW_NO_GLU)

