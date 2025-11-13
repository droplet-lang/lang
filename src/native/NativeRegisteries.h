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

const std::unordered_map<std::string, BuiltinInfo> ALL_NATIVE_FUNCTIONS = {
    {"exit", {"exit", Type::Void(), {}}},
    {"print", {"print", Type::Void(), {}}},
    {"println", {"println", Type::Void(), {}}},
    {"str", {"str", Type::String(), {Type::Unknown()}}},
    {"len", {"len", Type::Int(), {Type::Unknown()}}},
    {"int", {"int", Type::Int(), {Type::Unknown()}}},
    {"float", {"float", Type::Float(), {Type::Unknown()}}},
    {"input", {"input", Type::String(), {}}},
    {"android_native_toast", {"android_native_toast", Type::String(), {}}}
};

#endif //DROPLET_NATIVEREGISTERIES_H