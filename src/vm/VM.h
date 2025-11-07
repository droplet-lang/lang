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
 *  File: VM
 *  Created: 11/7/2025
 * ============================================================
 */
#ifndef DROPLET_VM_H
#define DROPLET_VM_H
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "defines.h"
#include "GC.h"
#include "Value.h"

struct VM;

struct Function {
    std::string name;
    std::vector<Value> constants;
    std::vector<uint8_t> code;
    uint8_t argCount = 0;
    uint8_t localCount = 0; // will include n(local_slots) + args
};

struct CallFrame {
    Function *function = nullptr;
    uint32_t ip = 0; // which line we are in, in the function code
    uint32_t localStartsAt = 0; // index in VM stack, where this frame locals starts
};

struct FFI {
    // Todo (@svpz) how do we load??
    // maybe we can track like:
    //  X ==== ptr to loaded X lib (pX)
    //  Y ==== ptr to loaded Y lib (pY)
    //  and then findSymbol(pX, "add")
};

struct VM {
    VM() {
        sp_internal = 0;
    }

    // stack
    std::vector<Value> stack;
    uint32_t sp = 0;

    // ops
    void push(const Value &value);
    Value pop();
    Value peek(int position) const;

    // internal stack container used by run
    std::vector<Value> stack_internal;
    uint32_t sp_internal = 0;
    uint32_t get_sp() const { return sp_internal; }

    // Stack ops
    void push_back(const Value &value);
    Value pop_back();
    Value peek_back(int position) const;


    // frames
    std::vector<CallFrame> call_frames;

    static uint8_t read_u8(CallFrame &frame);

    static uint8_t read_u16(CallFrame &frame);

    static uint8_t read_u32(CallFrame &frame);

    // "RETURN 2" means return top 2 value from stack
    // This is so that we can have go like err handling
    void do_return(uint8_t return_count);

    // global table
    std::unordered_map<std::string, Value> globals;

    // functions (loaded module)
    std::vector<std::unique_ptr<Function>> functions;
    std::unordered_map<std::string, uint32_t> function_index_by_name;

    uint32_t get_function_index(const std::string &name);
    void call_function_by_index(uint32_t fnIndex, size_t argCount);

    // Native registry
    std::unordered_map<std::string, NativeFunction> native_functions_registry;

    void register_native(const std::string &name, NativeFunction fn);

    // constants
    std::vector<Value> global_constants;

    // GC
    GC gc;

    // allocators
    ObjString *allocate_string(const std::string &str);
    ObjArray *allocate_array();
    ObjMap *allocate_map();
    ObjInstance *allocate_instance(const std::string &className);

    // stack root walker for GC
    void root_walker(const RWComplexGCFunction &walker);
    void collect_garbage_if_needed();
    void perform_gc();

    // loader
    static uint32_t read_u32(const std::vector<uint8_t> &buf, size_t &off);
    static uint16_t read_u16(const std::vector<uint8_t> &buf, size_t &off);
    static uint8_t read_u8(const std::vector<uint8_t> &buf, size_t &off);
    static double read_double(const std::vector<uint8_t> &buf, size_t &off);
    static int32_t read_i32(const std::vector<uint8_t> &buf, size_t &off);

    // loader and runner
    bool load_dbc_file(const std::string &path);
    void run(); // This is going to be huge rn, won-t refactor for clarity unless finalized
};


#endif //DROPLET_VM_H