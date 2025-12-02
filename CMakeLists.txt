cmake_minimum_required(VERSION 3.10)

project(main)
add_executable(main
        src/main.cpp
        src/implementations/ListMesh.cpp
        src/implementations/Triangle.cpp
        src/implementations/Matrix4.cpp
        src/implementations/Vector4.cpp
        src/implementations/Plain.cpp)
target_include_directories(main PRIVATE utils)
