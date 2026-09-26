cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Tells Clangd how to read your project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tfs_core
    src/term_set.cpp
    src/inputs.cpp
    src/path_handle.cpp
    src/search.cpp
)
target_include_directories(tfs_core PUBLIC src)

add_executable(Cexp src/main.cpp)
target_link_libraries(Cexp PRIVATE tfs_core)

add_executable(tests tests/test_search.cpp)
target_link_libraries(tests PRIVATE tfs_core)
