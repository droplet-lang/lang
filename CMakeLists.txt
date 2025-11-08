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
        src/tests/VM_test.h
        src/vm/DBC_helper.cpp
        src/vm/DBC_helper.h
        src/compiler/Lexer.cpp
        src/compiler/Lexer.h
        src/compiler/Parser.cpp
        src/compiler/Parser.h
        src/compiler/AST.cpp
        src/compiler/AST.h
        src/compiler/Semantic.cpp
        src/compiler/Semantic.h
        src/compiler/Compiler.cpp
        src/compiler/Compiler.h
)
