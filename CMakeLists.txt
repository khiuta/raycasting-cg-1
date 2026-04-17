cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(main)

# --- 1. ENCONTRAR A RAYLIB ---
# Isso procura os arquivos da raylib instalados no seu EndeavourOS
find_package(raylib REQUIRED)
find_package(glm REQUIRED)
# Opções de otimização (mantendo as suas)
add_compile_options(-Ofast)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")
find_package(OpenMP REQUIRED)

# --- 2. DEFINIR O EXECUTÁVEL ---
# Listando seus arquivos de implementação conforme seu projeto
add_executable(main
        src/main.cpp
        src/aux_functions.cpp
        src/implementations/ListMesh.cpp
        src/implementations/Triangle.cpp
        src/implementations/Matrix4.cpp
        src/implementations/Vector4.cpp
        src/implementations/Plain.cpp
        src/implementations/Sphere.cpp
        src/implementations/AABB.cpp
        src/implementations/Texture.cpp
        src/implementations/Cylinder.cpp
        src/implementations/Rectangle.cpp
        src/implementations/Cone.cpp
        src/implementations/Shader.cpp
        )
set(CMAKE_VERBOSE_MAKEFILE OFF)

# --- 3. DIRETÓRIOS DE INCLUDE ---
# Adiciona sua pasta utils e os cabeçalhos da raylib
target_include_directories(main PRIVATE utils)

# --- 4. VINCULAR BIBLIOTECAS (LINKER) ---
# Encontra o pacote X11 no Pop!_OS
find_package(X11 REQUIRED)

# Vincula a raylib e as dependências de sistema
target_link_libraries(main PRIVATE 
    raylib 
    ${X11_LIBRARIES}
    m 
    pthread 
    dl 
    rt
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
endif()

# Configuração de build (Release por padrão)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Configuração de build não especificada, definindo para 'Release'")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()