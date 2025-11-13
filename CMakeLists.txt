cmake_minimum_required(VERSION 4.0)
project(droplet)

set(CMAKE_CXX_STANDARD 20)

include_directories("D:/msys64/mingw64/include")
link_directories("D:/msys64/mingw64/lib")

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
        src/vm/DBC_helper.cpp
        src/vm/DBC_helper.h
        src/vm/FFIHelper.cpp
        src/vm/FFIHelper.h
        src/vm/StackManager.cpp
        src/vm/StackManager.h
        src/vm/CallFrame.cpp
        src/vm/CallFrame.h
        src/vm/Loader.cpp
        src/vm/Loader.h
        src/vm/Allocator.cpp
        src/vm/Allocator.h
        src/vm/Function.h
        src/compiler/Lexer.cpp
        src/compiler/Lexer.h
        src/compiler/Lexer.cpp
        src/tests/Lexer_test.h
        src/compiler/Expr.h
        src/compiler/Stmt.h
        src/compiler/Program.h
        src/compiler/Parser.cpp
        src/compiler/Parser.h
        src/compiler/CodeGenerator.cpp
        src/compiler/CodeGenerator.h
        src/compiler/ModuleLoader.cpp
        src/compiler/ModuleLoader.h
        src/compiler/TypeChecker.cpp
        src/compiler/TypeChecker.h
        src/native/Native.cpp
        src/native/Native.h
        src/debugger/Debugger.cpp
        src/debugger/Debugger.h
        src/native/AndroidNative.cpp
        src/native/AndroidNative.h
        src/native/NativeRegisteries.h
)

target_link_libraries(droplet PRIVATE ffi)
