//
// Created by NITRO on 11/13/2025.
//

#ifndef DROPLET_NATIVEREGISTERIES_H
#define DROPLET_NATIVEREGISTERIES_H
#include <string>
#include <vector>

// We need native to be freely added with as less code dependencies as possible
// Lexer, Parser, etc. takes the definition from here
struct BuiltinInfo {
    std::string name;
    std::shared_ptr<Type> returnType;
    std::vector<std::shared_ptr<Type>> paramTypes;
};

inline std::unordered_map<std::string, BuiltinInfo> ALL_NATIVE_FUNCTIONS;

// Helper to register a function
inline void registerNative(const BuiltinInfo& func) {
    ALL_NATIVE_FUNCTIONS[func.name] = func;
}

inline void initCoreBuiltins() {
    registerNative({"exit", Type::Void(), {}});
    registerNative({"print", Type::Void(), {}});
    registerNative({"println", Type::Void(), {}});
    registerNative({"str", Type::String(), {Type::Unknown()}});
    registerNative({"len", Type::Int(), {Type::Unknown()}});
    registerNative({"int", Type::Int(), {Type::Unknown()}});
    registerNative({"float", Type::Float(), {Type::Unknown()}});
    registerNative({"input", Type::String(), {}});
}

#endif //DROPLET_NATIVEREGISTERIES_H