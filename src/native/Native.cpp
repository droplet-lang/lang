/*
 * ============================================================
 *  Droplet 
 * ============================================================
 *  Copyright (c) 2025 Droplet Contributors
 *  All rights reserved.
 *
 *  Licensed under the MIT License.
 *  See LICENSE file in the project root for full license.
 *
 *  File: Native
 *  Created: 11/9/2025
 * ============================================================
 */
#include "Native.h"

#include <iostream>

#include "../vm/VM.h"

void native_print(VM& vm, const uint8_t argc) {
    for (int i = argc - 1; i >= 0; i--) {
        Value v = vm.stack_manager.peek(i);
        std::cout << v.toString();
        if (i > 0) std::cout << " ";
    }

    for (int i = 0; i < argc; i++) {
        vm.stack_manager.pop();
    }

    vm.stack_manager.push(Value::createNIL());
}

// Native println function (with newline)
void native_println(VM& vm, const uint8_t argc) {
    for (int i = argc - 1; i >= 0; i--) {
        Value v = vm.stack_manager.peek(i);
        std::cout << v.toString();
        if (i > 0) std::cout << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < argc; i++) {
        vm.stack_manager.pop();
    }

    vm.stack_manager.push(Value::createNIL());
}

// Native str function
void native_str(VM& vm, const uint8_t argc) {
    if (argc != 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    Value v = vm.stack_manager.pop();
    ObjString* str = vm.allocator.allocate_string(v.toString());
    vm.stack_manager.push(Value::createOBJECT(str));
}

// Native len function
void native_len(VM& vm, const uint8_t argc) {
    if (argc != 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createINT(0));
        return;
    }

    const Value v = vm.stack_manager.pop();
    if (v.type == ValueType::OBJECT && v.current_value.object) {
        if (const auto* arr = dynamic_cast<ObjArray*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(arr->value.size())));
            return;
        }
        if (const auto* map = dynamic_cast<ObjMap*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(map->value.size())));
            return;
        }
        if (const auto* str = dynamic_cast<ObjString*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(str->value.size())));
            return;
        }
    }
    vm.stack_manager.push(Value::createINT(0));
}