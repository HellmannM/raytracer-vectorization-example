cmake_minimum_required(VERSION 3.22)

project(raytracer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG COMPONENTS filesystem iostreams system REQUIRED)
find_package(PNG REQUIRED)

add_executable(raytracer)

target_sources(raytracer PRIVATE
    3rdparty/CmdLine/src/CmdLine.cpp
    3rdparty/CmdLine/src/CmdLineUtil.cpp
    common/file_base.cpp
    common/image.cpp
    common/image_base.cpp
    common/model.cpp
    common/obj_grammar.cpp
    common/obj_loader.cpp
    common/pixel_format.cpp
    common/png_image.cpp
    common/sg.cpp
    main.cpp
)

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
)

target_link_libraries(raytracer PRIVATE
    Boost::disable_autolinking
    Boost::filesystem
    Boost::iostreams
    PNG::PNG
)

