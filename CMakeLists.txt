cmake_minimum_required(VERSION 4.0)
project(droplet)

set(CMAKE_CXX_STANDARD 20)

add_executable(droplet main.cpp
        src/vm/VM.cpp
        src/vm/VM.h
        src/vm/Value.cpp
        src/vm/Value.h
        src/vm/Object.h
        src/vm/GC.cpp
        src/vm/GC.h
        src/vm/defines.h
        src/external/dlfcn/dlfcn.h
        src/external/dlfcn/dlfcn.c
        src/tests/GC_test.h
        src/tests/Value_obj_test.h
)
