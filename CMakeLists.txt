cmake_minimum_required(VERSION 4.0)
project(droplet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories("D:/msys64/mingw64/include")

# Link directories
link_directories("D:/msys64/mingw64/lib")

# Collect source files recursively
file(GLOB_RECURSE DROPLET_SOURCES
        src/vm/*.cpp
        src/compiler/*.cpp
        src/native/*.cpp
        src/debugger/*.cpp
        src/external/dlfcn/*.c
        main.cpp
)

file(GLOB_RECURSE DROPLET_HEADERS
        src/vm/*.h
        src/compiler/*.h
        src/native/*.h
        src/debugger/*.h
        src/external/dlfcn/*.h
        src/tests/*.h
)

# Create executable
add_executable(droplet ${DROPLET_SOURCES} ${DROPLET_HEADERS})

# Link libraries
target_link_libraries(droplet PRIVATE ffi)
