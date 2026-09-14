cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Tells Clangd how to read your project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Cexp
    src/main.cpp
    src/term_set.h
    src/term_set.cpp
    src/inputs.h
    src/inputs.cpp
    src/path_handle.h
    src/path_handle.cpp
    src/search.h
    src/search.cpp
)
