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
    // Core functions
    registerNative({"exit", Type::Void(), {}});
    registerNative({"print", Type::Void(), {}});
    registerNative({"println", Type::Void(), {}});
    registerNative({"str", Type::String(), {Type::Unknown()}});
    registerNative({"len", Type::Int(), {Type::Unknown()}});
    registerNative({"int", Type::Int(), {Type::Unknown()}});
    registerNative({"float", Type::Float(), {Type::Unknown()}});
    registerNative({"input", Type::String(), {}});
    registerNative({"append", Type::Void(), {Type::List(Type::Unknown()), Type::Unknown()}});
    registerNative({"forEach", Type::Void(), {Type::Unknown()}});

    // Android basic functions
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

    // Text Input (NEW)
    registerNative({"android_create_edittext", Type::Int(), {}});
    registerNative({"android_get_edittext_value", Type::String(), {}});
    registerNative({"android_set_edittext_hint", Type::Null(), {}});
    registerNative({"android_set_edittext_input_type", Type::Null(), {}});

    // View manipulation
    registerNative({"android_add_view_to_parent", Type::Null(), {}});
    registerNative({"android_set_view_text", Type::Null(), {}});
    registerNative({"android_set_view_image", Type::Null(), {}});
    registerNative({"android_set_view_visibility", Type::Null(), {}});
    registerNative({"android_set_view_background_color", Type::Null(), {}});
    registerNative({"android_set_view_padding", Type::Null(), {}});
    registerNative({"android_set_view_size", Type::Null(), {}});

    // Styling (NEW)
    registerNative({"android_set_text_size", Type::Null(), {}});
    registerNative({"android_set_text_color", Type::Null(), {}});
    registerNative({"android_set_text_style", Type::Null(), {}});
    registerNative({"android_set_view_margin", Type::Null(), {}});
    registerNative({"android_set_view_gravity", Type::Null(), {}});
    registerNative({"android_set_view_elevation", Type::Null(), {}});
    registerNative({"android_set_view_corner_radius", Type::Null(), {}});
    registerNative({"android_set_view_border", Type::Null(), {}});

    // RecyclerView specific
    registerNative({"android_recyclerview_add_item", Type::Null(), {}});
    registerNative({"android_recyclerview_clear", Type::Null(), {}});

    // Toolbar and Navigation
    registerNative({"android_set_toolbar_title", Type::Null(), {}});
    registerNative({"android_create_screen", Type::Int(), {}});
    registerNative({"android_navigate_to_screen", Type::Null(), {}});
    registerNative({"android_navigate_back", Type::Null(), {}});
    registerNative({"android_set_back_button_visible", Type::Null(), {}});
}

#endif //DROPLET_NATIVEREGISTERIES_H