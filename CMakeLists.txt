cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(main)
add_compile_options(-Ofast)
add_compile_options(-g)
add_executable(main
        src/main.cpp
        src/implementations/ListMesh.cpp
        src/implementations/Triangle.cpp
        src/implementations/Matrix4.cpp
        src/implementations/Vector4.cpp
        src/implementations/Plain.cpp
        src/implementations/Sphere.cpp
        src/implementations/AABB.cpp
        src/implementations/Texture.cpp)
target_include_directories(main PRIVATE utils)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Configuração de build não especificada, definindo para 'Release'")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Define as opções disponíveis no cache do CMake-GUI
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
