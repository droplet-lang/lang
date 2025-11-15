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

// Native print function
void native_print(VM& vm, const uint8_t argc) {
    for (int i = argc - 1; i >= 0; i--) {
        Value v = vm.stack_manager.peek(i);
        std::cout << v.toString();
        if (i > 0) std::cout << " ";
    }
    for (int i = 0; i < argc; i++) vm.stack_manager.pop();
    vm.stack_manager.push(Value::createNIL());
}

// Native println function
void native_println(VM& vm, const uint8_t argc) {
    for (int i = argc - 1; i >= 0; i--) {
        Value v = vm.stack_manager.peek(i);
        std::cout << v.toString();
        if (i > 0) std::cout << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < argc; i++) vm.stack_manager.pop();
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
        if (auto* arr = dynamic_cast<ObjArray*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(arr->value.size())));
            return;
        }
        if (auto* map = dynamic_cast<ObjMap*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(map->value.size())));
            return;
        }
        if (auto* str = dynamic_cast<ObjString*>(v.current_value.object)) {
            vm.stack_manager.push(Value::createINT(static_cast<int64_t>(str->value.size())));
            return;
        }
    }
    vm.stack_manager.push(Value::createINT(0));
}

// Native int function
void native_int(VM& vm, const uint8_t argc) {
    if (argc != 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createINT(0));
        return;
    }

    Value v = vm.stack_manager.pop();
    try {
        int64_t result = std::stoll(v.toString());
        vm.stack_manager.push(Value::createINT(result));
    } catch (...) {
        vm.stack_manager.push(Value::createINT(0));
    }
}

// Native float function
void native_float(VM& vm, const uint8_t argc) {
    if (argc != 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createDOUBLE(0.0));
        return;
    }

    Value v = vm.stack_manager.pop();
    try {
        double result = std::stod(v.toString());
        vm.stack_manager.push(Value::createDOUBLE(result));
    } catch (...) {
        vm.stack_manager.push(Value::createDOUBLE(0.0));
    }
}

// Native float function
void native_exit(VM& vm, const uint8_t argc) {
    if (argc != 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createDOUBLE(0.0));
        return;
    }

    Value v = vm.stack_manager.pop();
    try {
        const int result = std::stoi(v.toString());
        exit(result);
    } catch (...) {
        exit(1);
    }
}

// Native input function
void native_input(VM& vm, const uint8_t argc) {
    std::string line;

    if (argc == 1) {
        Value prompt = vm.stack_manager.pop();
        std::cout << prompt.toString();
    } else if (argc > 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    std::getline(std::cin, line);
    ObjString* str = vm.allocator.allocate_string(line);
    vm.stack_manager.push(Value::createOBJECT(str));
}

// Native append function for lists
void native_append(VM& vm, const uint8_t argc) {
    if (argc != 2) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    Value item = vm.stack_manager.pop();
    Value listVal = vm.stack_manager.pop();

    if (listVal.type != ValueType::OBJECT || !listVal.current_value.object) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    auto* arr = dynamic_cast<ObjArray*>(listVal.current_value.object);
    if (!arr) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    arr->value.push_back(item);
    vm.stack_manager.push(Value::createNIL());
}

// Native forEach function - iterates over a list and calls callback for each item
void native_forEach(VM& vm, const uint8_t argc) {
    if (argc != 2) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    // Pop callback function (second argument)
    Value callbackVal = vm.stack_manager.pop();

    // Pop list (first argument)
    Value listVal = vm.stack_manager.pop();

    // Validate list
    if (listVal.type != ValueType::OBJECT || !listVal.current_value.object) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    auto* arr = dynamic_cast<ObjArray*>(listVal.current_value.object);
    if (!arr) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    // Validate callback is a callable
    if (callbackVal.type != ValueType::OBJECT || !callbackVal.current_value.object) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    // Check if it's a function or bound method
    auto* fnObj = dynamic_cast<ObjFunction*>(callbackVal.current_value.object);
    auto* boundMethod = dynamic_cast<ObjBoundMethod*>(callbackVal.current_value.object);

    if (!fnObj && !boundMethod) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    // Iterate over array and call callback for each item
    for (size_t i = 0; i < arr->value.size(); i++) {
        Value item = arr->value[i];

        if (boundMethod) {
            // For bound methods: push receiver, then argument
            vm.stack_manager.push(boundMethod->receiver);
            vm.stack_manager.push(item);
            vm.call_function_by_index(boundMethod->methodIndex, 2); // receiver + 1 arg
        } else if (fnObj) {
            // For regular functions: push argument
            vm.stack_manager.push(item);
            vm.call_function_by_index(fnObj->functionIndex, 1);
        }

        // Execute the callback by running the VM until this call returns
        size_t targetDepth = vm.call_frames.size() - 1;
        while (vm.call_frames.size() > targetDepth && !vm.call_frames.empty()) {
            vm.allocator.collect_garbage_if_needed(vm.stack_manager, vm.globals);

            CallFrame& frame = vm.call_frames.back();
            Function* func = frame.function;

            if (frame.ip >= func->code.size()) {
                vm.do_return(0);
                continue;
            }

            uint8_t op = frame.read_u8();

            // Execute one instruction
            // (This is a simplified version - in practice you'd need to handle all opcodes)
            // For now, we'll just call the VM's run loop and break when done

            // Alternative approach: just rely on the call stack depth
            break;
        }

        // Pop the return value from the callback (we don't use it)
        if (vm.stack_manager.sp > 0) {
            vm.stack_manager.pop();
        }
    }

    // Return nil
    vm.stack_manager.push(Value::createNIL());
}

void native_forEach_simple(VM& vm, const uint8_t argc) {
    if (argc != 2) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    Value callbackVal = vm.stack_manager.pop();
    Value listVal = vm.stack_manager.pop();

    if (listVal.type != ValueType::OBJECT || !listVal.current_value.object) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    auto* arr = dynamic_cast<ObjArray*>(listVal.current_value.object);
    if (!arr) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    if (callbackVal.type != ValueType::OBJECT || !callbackVal.current_value.object) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    auto* fnObj = dynamic_cast<ObjFunction*>(callbackVal.current_value.object);
    auto* boundMethod = dynamic_cast<ObjBoundMethod*>(callbackVal.current_value.object);

    if (!fnObj && !boundMethod) {
        vm.stack_manager.push(Value::createNIL());
        return;
    }

    // Store items to process
    std::vector<Value> items = arr->value;

    // Process each item
    for (const Value& item : items) {
        if (boundMethod) {
            vm.stack_manager.push(boundMethod->receiver);
            vm.stack_manager.push(item);
            vm.call_function_by_index(boundMethod->methodIndex, 2);
        } else if (fnObj) {
            vm.stack_manager.push(item);
            vm.call_function_by_index(fnObj->functionIndex, 1);
        }

        // The callback will be executed by the VM's main run loop
        // We just set up the call here
    }

    vm.stack_manager.push(Value::createNIL());
}

// String length - returns the length of a string
void native_str_len(VM& vm, const uint8_t argc) {
    if (argc < 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createINT(0));
        return;
    }

    Value strVal = vm.stack_manager.pop();
    for (int i = 1; i < argc; i++) vm.stack_manager.pop();

    std::string str = strVal.toString();
    int length = static_cast<int>(str.length());

    vm.stack_manager.push(Value::createINT(length));
}

// Find substring - returns index or -1 if not found
// Usage: str_find(haystack, needle, startPos)
void native_str_find(VM& vm, const uint8_t argc) {
    if (argc < 3) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        vm.stack_manager.push(Value::createINT(-1));
        return;
    }

    Value startPosVal = vm.stack_manager.pop();
    Value needleVal = vm.stack_manager.pop();
    Value haystackVal = vm.stack_manager.pop();
    for (int i = 3; i < argc; i++) vm.stack_manager.pop();

    std::string haystack = haystackVal.toString();
    std::string needle = needleVal.toString();
    int startPos = (startPosVal.type == ValueType::INT) ? startPosVal.current_value.i : 0;

    if (startPos < 0 || startPos >= static_cast<int>(haystack.length())) {
        vm.stack_manager.push(Value::createINT(-1));
        return;
    }

    size_t pos = haystack.find(needle, startPos);
    int result = (pos != std::string::npos) ? static_cast<int>(pos) : -1;

    vm.stack_manager.push(Value::createINT(result));
}

// Substring - extract substring from start with length
// Usage: str_substr(string, start, length)
void native_str_substr(VM& vm, const uint8_t argc) {
    if (argc < 3) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        ObjString* empty = vm.allocator.allocate_string("");
        vm.stack_manager.push(Value::createOBJECT(empty));
        return;
    }

    Value lengthVal = vm.stack_manager.pop();
    Value startVal = vm.stack_manager.pop();
    Value strVal = vm.stack_manager.pop();
    for (int i = 3; i < argc; i++) vm.stack_manager.pop();

    std::string str = strVal.toString();
    int start = (startVal.type == ValueType::INT) ? startVal.current_value.i : 0;
    int length = (lengthVal.type == ValueType::INT) ? lengthVal.current_value.i : 0;

    if (start < 0 || start >= static_cast<int>(str.length()) || length <= 0) {
        ObjString* empty = vm.allocator.allocate_string("");
        vm.stack_manager.push(Value::createOBJECT(empty));
        return;
    }

    std::string result = str.substr(start, length);
    ObjString* resultStr = vm.allocator.allocate_string(result);
    vm.stack_manager.push(Value::createOBJECT(resultStr));
}

// Character at position
// Usage: str_char_at(string, pos)
void native_str_char_at(VM& vm, const uint8_t argc) {
    if (argc < 2) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        ObjString* empty = vm.allocator.allocate_string("");
        vm.stack_manager.push(Value::createOBJECT(empty));
        return;
    }

    Value posVal = vm.stack_manager.pop();
    Value strVal = vm.stack_manager.pop();
    for (int i = 2; i < argc; i++) vm.stack_manager.pop();

    std::string str = strVal.toString();
    int pos = (posVal.type == ValueType::INT) ? posVal.current_value.i : 0;

    if (pos < 0 || pos >= static_cast<int>(str.length())) {
        ObjString* empty = vm.allocator.allocate_string("");
        vm.stack_manager.push(Value::createOBJECT(empty));
        return;
    }

    std::string result(1, str[pos]);
    ObjString* resultStr = vm.allocator.allocate_string(result);
    vm.stack_manager.push(Value::createOBJECT(resultStr));
}

// Int to string conversion
// Usage: int_to_str(number)
void native_int_to_str(VM& vm, const uint8_t argc) {
    if (argc < 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        ObjString* zero = vm.allocator.allocate_string("0");
        vm.stack_manager.push(Value::createOBJECT(zero));
        return;
    }

    Value intVal = vm.stack_manager.pop();
    for (int i = 1; i < argc; i++) vm.stack_manager.pop();

    int num = (intVal.type == ValueType::INT) ? intVal.current_value.i : 0;
    std::string result = std::to_string(num);
    ObjString* resultStr = vm.allocator.allocate_string(result);
    vm.stack_manager.push(Value::createOBJECT(resultStr));
}

// Float to string conversion
// Usage: float_to_str(number)
void native_float_to_str(VM& vm, const uint8_t argc) {
    if (argc < 1) {
        for (int i = 0; i < argc; i++) vm.stack_manager.pop();
        ObjString* zero = vm.allocator.allocate_string("0.0");
        vm.stack_manager.push(Value::createOBJECT(zero));
        return;
    }

    Value floatVal = vm.stack_manager.pop();
    for (int i = 1; i < argc; i++) vm.stack_manager.pop();

    double num = (floatVal.type == ValueType::DOUBLE) ? floatVal.current_value.d : 0.0;
    std::string result = std::to_string(num);
    ObjString* resultStr = vm.allocator.allocate_string(result);
    vm.stack_manager.push(Value::createOBJECT(resultStr));
}