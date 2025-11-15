//
// Created by NITRO on 11/13/2025.
//

#ifndef DROPLET_NATIVEREGISTERIES_H
#define DROPLET_NATIVEREGISTERIES_H
#include <string>
#include <vector>
#include "../compiler/TypeChecker.h"

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
    registerNative({"android_create_button", Type::String(), {}});
    registerNative({"android_native_toast", Type::String(), {}});
    registerNative({"append", Type::Void(), {Type::List(Type::Unknown()), Type::Unknown()}});
    registerNative({"forEach", Type::Void(), {Type::Unknown()}});

    // String manipulation functions
    registerNative({"str_len", Type::Int(), {Type::String()}});
    registerNative({"str_find", Type::Int(), {Type::String(), Type::String(), Type::Int()}});
    registerNative({"str_substr", Type::String(), {Type::String(), Type::Int(), Type::Int()}});
    registerNative({"str_char_at", Type::String(), {Type::String(), Type::Int()}});
    registerNative({"int_to_str", Type::String(), {Type::Int()}});
    registerNative({"float_to_str", Type::String(), {Type::Float()}});

    registerNative({"android_create_button", Type::String(), {}});
    registerNative({"android_native_toast", Type::String(), {}});

    // Layout views
    registerNative({"android_create_linearlayout", Type::Int(), {}});
    registerNative({"android_create_scrollview", Type::Int(), {}});
    registerNative({"android_create_cardview", Type::Int(), {}});
    registerNative({"android_create_recyclerview", Type::Int(), {}});

    // Basic views
    registerNative({"android_create_textview", Type::Int(), {}});
    registerNative({"android_create_imageview", Type::Int(), {}});

    // View manipulation
    registerNative({"android_add_view_to_parent", Type::Null(), {}});
    registerNative({"android_set_view_text", Type::Null(), {}});
    registerNative({"android_set_view_image", Type::Null(), {}});
    registerNative({"android_set_view_visibility", Type::Null(), {}});
    registerNative({"android_set_view_background_color", Type::Null(), {}});
    registerNative({"android_set_view_padding", Type::Null(), {}});
    registerNative({"android_set_view_size", Type::Null(), {}});

    // RecyclerView specific
    registerNative({"android_recyclerview_add_item", Type::Null(), {}});
    registerNative({"android_recyclerview_clear", Type::Null(), {}});

    registerNative({"android_http_get", Type::Null(), {}});
    registerNative({"android_http_post", Type::Null(), {}});
    registerNative({"android_http_put", Type::Null(), {}});
    registerNative({"android_http_delete", Type::Null(), {}});
}

#endif //DROPLET_NATIVEREGISTERIES_H