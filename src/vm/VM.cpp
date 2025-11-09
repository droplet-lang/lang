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
#include "VM.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include "../external/dlfcn/dlfcn.h"

void *FFI::load_lib(const std::string &path) {
    auto it = libs.find(path);
    if (it != libs.end()) return it->second;


    void *h = dlopen(path.c_str(), RTLD_NOW);
    if (!h) {
        std::cerr << "[FFI] dlopen failed: " << dlerror() << '\n';
        return nullptr;
    }

    libs[path] = h;
    return h;
}

void *FFI::find_symbol(void *lib, const std::string &symbol) {
    if (!lib) return nullptr;

    void *p = dlsym(lib, symbol.c_str());
    return p;
}

void VM::push(const Value &value) {
    if (sp >= stack.size()) {
        stack.push_back(value);
    } else {
        stack[sp] = value;
        sp++;
    }
}

Value VM::pop() {
    if (sp == 0) return Value::createNIL();
    sp--;
    return stack[sp];
}

Value VM::peek(const int position) const {
    const int idx = static_cast<int>(sp) - 1 - position;
    if (idx < 0) return Value::createNIL();
    return stack[idx];
}

void VM::push_back(const Value &value) {
    if (sp_internal >= stack_internal.size()) stack_internal.push_back(value);
    else stack_internal[sp_internal] = value;
    sp_internal++;
}

Value VM::pop_back() {
    if (sp_internal == 0) return Value::createNIL();
    sp_internal--;
    return stack_internal[sp_internal];
}

Value VM::peek_back(const int position) const {
    const int idx = static_cast<int>(sp_internal) - 1 - position;
    if (idx < 0) return Value::createNIL();
    return stack_internal[idx];
}

uint8_t VM::read_u8(CallFrame &frame) {
    const uint8_t v = frame.function->code[frame.ip++];
    return v;
}

uint8_t VM::read_u16(CallFrame &frame) {
    const auto &c = frame.function->code;
    const uint16_t v = static_cast<uint16_t>(c[frame.ip]) | (static_cast<uint16_t>(c[frame.ip + 1]) << 8);
    frame.ip += 2;
    return v;
}

uint8_t VM::read_u32(CallFrame &frame) {
    const auto &c = frame.function->code;
    const uint32_t v = static_cast<uint32_t>(c[frame.ip]) | (static_cast<uint32_t>(c[frame.ip + 1]) << 8) | (
                           static_cast<uint32_t>(c[frame.ip + 2]) << 16) | (
                           static_cast<uint32_t>(c[frame.ip + 3]) << 24);
    frame.ip += 4;
    return v;
}

void VM::do_return(const uint8_t return_count) {
    if (call_frames.empty()) return;

    // Collect return values
    std::vector<Value> rets;
    for (int i = 0; i < return_count; i++) {
        rets.push_back(pop_back());
    }

    const CallFrame frame = call_frames.back();
    call_frames.pop_back();

    // CORRECT FIX: The returning function's locals start at frame.localStartsAt
    // We need to restore sp_internal to that position, which will:
    // 1. Remove all the function's locals (arguments + additional locals)
    // 2. Leave the caller's stack intact (because localStartsAt points to where
    //    this function's data begins, which is AFTER the caller's data)

    const uint32_t restore = frame.localStartsAt;

    // Restore stack pointer to before this function's locals
    sp_internal = restore;


    // Push return values
    for (int i = static_cast<int>(rets.size()) - 1; i >= 0; i--) {
        push_back(rets[i]);
    }

}

uint32_t VM::get_function_index(const std::string &name) {
    const auto it = function_index_by_name.find(name);
    if (it == function_index_by_name.end()) return UINT32_MAX;
    return it->second;
}

void VM::call_function_by_index(const uint32_t fnIndex, size_t argCount) {
    if (fnIndex >= functions.size()) {
        std::cerr << "#########call_function_by_index: bad index\n";
        return;
    }

    Function *f = functions[fnIndex].get();
    CallFrame frame;
    frame.function = f;
    frame.ip = 0;
    frame.localStartsAt = get_sp();
    call_frames.push_back(frame);
}

void VM::register_native(const std::string &name, NativeFunction fn) {
    native_functions_registry[name] = fn;
}

ObjString *VM::allocate_string(const std::string &str) {
    auto *o = new ObjString(str);
    gc.allocNewObject(o);
    return o;
}

ObjArray *VM::allocate_array() {
    auto *o = new ObjArray();
    gc.allocNewObject(o);
    return o;
}

ObjMap *VM::allocate_map() {
    auto *o = new ObjMap();
    gc.allocNewObject(o);
    return o;
}

ObjInstance *VM::allocate_instance(const std::string &className) {
    auto *o = new ObjInstance(className);
    gc.allocNewObject(o);
    return o;
}

void VM::root_walker(const RWComplexGCFunction &walker) {
    walker([this](const Value v) {
        gc.markValue(v);
    });
}

void VM::collect_garbage_if_needed() {
    if (gc.heap.size() > gc.memThresholdForNextGcCall) {
        perform_gc();
    }
}

void VM::perform_gc() {
    gc.collect([this](const MarkerFunction &mark) {
        // stack (frame ko local are already part of stack)
        // so this must cover stack + frame local
        for (uint32_t i = 0; i < sp; i++) mark(stack[i]);

        // globals
        for (const auto &val: globals) mark(val.second);
        // function constants (if we used global constants)
        // nothing else
    });
}

uint32_t VM::read_u32(const std::vector<uint8_t> &buf, size_t &off) {
    const uint32_t v = static_cast<uint32_t>(buf[off]) | (static_cast<uint32_t>(buf[off + 1]) << 8) | (
                           static_cast<uint32_t>(buf[off + 2]) << 16) | (static_cast<uint32_t>(buf[off + 3]) << 24);
    off += 4;
    return v;
}

uint16_t VM::read_u16(const std::vector<uint8_t> &buf, size_t &off) {
    const uint16_t v = static_cast<uint16_t>(buf[off]) | (static_cast<uint16_t>(buf[off + 1]) << 8);
    off += 2;
    return v;
}

uint8_t VM::read_u8(const std::vector<uint8_t> &buf, size_t &off) {
    const uint8_t v = buf[off];
    off += 1;
    return v;
}

double VM::read_double(const std::vector<uint8_t> &buf, size_t &off) {
    double x;
    memcpy(&x, &buf[off], sizeof(double));
    off += sizeof(double);
    return x;
}

int32_t VM::read_i32(const std::vector<uint8_t> &buf, size_t &off) {
    int32_t v;
    memcpy(&v, &buf[off], sizeof(int32_t));
    off += 4;
    return v;
}

bool VM::load_dbc_file(const std::string &path) {
    /*
     * The dbc file will be the one containing information about constants, functions and code
     * It's format:
     *
     *     |  HEADER "DLBC" (4)           |    VERSION (1)    |
     *     |  constant_count (u32t)       |    [...constants] |
     *     |  function_count (u32t)       |    [...fn header] |
     *     |  code_size (u32t)            |    [...byte code] |
     *
     * Here:
     *
     * [...constants]
     *      TYPE (u8t) == which indicate size of this constant in consequitive byte
     *          1 = i32t
     *          2 = double
     *          3 = u32t len; bytes[len] (for string)
     *          4 = no data (NIL0
     *          5 = u8t(0/1 - BOOL)
     *
     * [...fn header]
     *      u32t nameIndex
     *      u32t start
     *      u32t size
     *      u8t  argCount
     *      u8t  localCount
    */
    std::ifstream f(path, std::ios::binary);

    if (!f.is_open()) {
        std::cerr << "Failed to open " << path << "\n";
        return false;
    }

    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t off = 0;
    if (buf.size() < 5) {
        std::cerr << "Invalid dbc\n";
        return false;
    }

    // check if there is a correct magic string
    std::string magic(reinterpret_cast<char *>(&buf[off]), 4);
    off += 4;

    if (magic != "DLBC") {
        std::cerr << "Bad magic\n";
        return false;
    }

    // test curr version
    uint8_t version = read_u8(buf, off);
    if (version != 1) {
        // Todo (@svpz) hardcoded version doesnot make sense
        std::cerr << "Unsupported version\n";
        return false;
    }

    // constants (global pool)
    uint32_t constCount = read_u32(buf, off);

    // create a list that can hold up constants
    std::vector<Value> constPool;
    constPool.reserve(constCount);

    for (uint32_t i = 0; i < constCount; i++) {
        uint8_t t = read_u8(buf, off);

        if (t == 1) {
            // INT
            int32_t vi = read_i32(buf, off);
            constPool.push_back(Value::createINT(vi));
        } else if (t == 2) {
            // DOUBLE
            double dv = read_double(buf, off);
            constPool.push_back(Value::createDOUBLE(dv));
        } else if (t == 3) {
            // STRING
            uint32_t len = read_u32(buf, off);
            std::string s(reinterpret_cast<char *>(&buf[off]), len);
            off += len;
            ObjString *os = allocate_string(s);
            constPool.push_back(Value::createOBJECT(os));
        } else if (t == 4) {
            // NIL
            constPool.push_back(Value::createNIL());
        } else if (t == 5) {
            // BOOL
            uint8_t b = read_u8(buf, off);
            constPool.push_back(Value::createBOOL(b != 0));
        } else {
            std::cerr << "Unknown const type " << static_cast<int>(t) << "\n";
            return false;
        }
    }

    // functions
    uint32_t fnCount = read_u32(buf, off);

    struct FnHeader {
        uint32_t nameIndex;
        uint32_t start;
        uint32_t size;
        uint8_t argCount;
        uint8_t localCount;
    };

    std::vector<FnHeader> headers;
    headers.reserve(fnCount);
    for (uint32_t i = 0; i < fnCount; i++) {
        FnHeader h;
        h.nameIndex = read_u32(buf, off);
        h.start = read_u32(buf, off);
        h.size = read_u32(buf, off);
        h.argCount = read_u8(buf, off);
        h.localCount = read_u8(buf, off);
        headers.push_back(h);
    }

    // code section size
    uint32_t codeSize = read_u32(buf, off);

    if (off + codeSize > buf.size()) {
        std::cerr << "Bad code size\n";
        return false;
    }

    std::vector code(buf.begin() + off, buf.begin() + off + codeSize);
    off += codeSize;

    // create Function entries, each referring to its sub-slice of code and constants.
    for (uint32_t i = 0; i < fnCount; i++) {
        FnHeader &h = headers[i];
        // name from constPool[nameIndex]
        if (h.nameIndex >= constPool.size() || constPool[h.nameIndex].type != ValueType::OBJECT) {
            std::cerr << "Invalid name index for function header\n";
            return false;
        }

        auto on = dynamic_cast<ObjString *>(constPool[h.nameIndex].current_value.object);
        if (!on) {
            std::cerr << "Function name not string\n";
            return false;
        }

        std::string fname = on->value;
        auto fn = std::make_unique<Function>();

        fn->name = fname;
        fn->argCount = h.argCount;
        fn->localCount = h.localCount;

        // For simplicity: all function constants will be empty here (we use global constants only)
        // copy the code slice
        if (h.start + h.size > code.size()) {
            std::cerr << "Function code out of bounds\n";
            return false;
        }

        fn->code.assign(code.begin() + h.start, code.begin() + h.start + h.size);

        // add to function table
        uint32_t idx = static_cast<uint32_t>(functions.size());
        function_index_by_name[fn->name] = idx;
        functions.push_back(std::move(fn));
    }

    // for now we store global constants (strings) as globalConstants
    for (auto &c: constPool) global_constants.push_back(c);

    std::cerr << "Loaded module '" << path << "' functions=" << fnCount << " constants=" << constCount << " code=" <<
            codeSize << "\n";
    return true;
}

void VM::run() {
    // GC GC GC
    while (!call_frames.empty()) {
        collect_garbage_if_needed();

        CallFrame &frame = call_frames.back();
        Function *func = frame.function;
        if (frame.ip >= func->code.size()) {
            do_return(0);
            continue;
        }

        uint8_t op = read_u8(frame);
        switch (static_cast<Op>(op)) {
            case OP_PUSH_CONST: {
                const uint32_t idx = read_u32(frame);
                if (idx >= global_constants.size()) {
                    push_back(Value::createNIL());
                    break;
                }
                push_back(global_constants[idx]);
                break;
            }
            case OP_POP: {
                pop_back();
                break;
            }
            case OP_CALL: {
                uint32_t fnIdx = read_u32(frame);
                uint8_t argc = read_u8(frame);

                if (fnIdx >= functions.size()) {
                    // pop args
                    for (int i = 0; i < argc; i++) pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                Function *callee = functions[fnIdx].get();

                // The arguments are on the stack at positions [sp_internal - argc .. sp_internal - 1]
                // These arguments become the first 'argc' locals of the called function
                // localStartsAt points to where local 0 begins (which is the first argument)

                CallFrame newFrame;
                newFrame.function = callee;
                newFrame.ip = 0;
                newFrame.localStartsAt = sp_internal - argc;


                // CRITICAL FIX: Ensure we have space for all locals (not just args)
                // The function might have more locals than just the arguments
                // We need to allocate space for localCount locals total

                // Arguments occupy slots [0 .. argc-1]
                // Additional locals occupy slots [argc .. localCount-1]
                uint8_t additionalLocals = callee->localCount > argc ? callee->localCount - argc : 0;


                // Push NIL for additional local variable slots
                for (uint8_t i = 0; i < additionalLocals; i++) {
                    push_back(Value::createNIL());
                }

                call_frames.push_back(newFrame);
                break;
            }

            case OP_LOAD_LOCAL: {
                uint8_t slot = read_u8(frame);
                uint32_t abs = frame.localStartsAt + slot;


                if (abs < sp_internal) {
                    Value val = stack_internal[abs];
                    push_back(val);
                } else {
                    push_back(Value::createNIL());
                }
                break;
            }
            case OP_STORE_LOCAL: {
                uint8_t slot = read_u8(frame);
                uint32_t abs = frame.localStartsAt + slot;
                Value val = pop_back();
                // ensure stack has space
                while (sp_internal <= abs) push_back(Value::createNIL());
                stack_internal[abs] = val;
                break;
            }
            case OP_DUP: {
                Value v = peek_back(0);
                push_back(v);
                break;
            }
            case OP_SWAP: {
                Value a = pop_back();
                Value b = pop_back();
                push_back(a);
                push_back(b);
                break;
            }
            case OP_ROT: {
                // rotate top 3: a b c -> b c a
                Value a = pop_back();
                Value b = pop_back();
                Value c = pop_back();
                push_back(b);
                push_back(a);
                push_back(c);
                break;
            }

            // Arithmetic
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_MOD: {
                Value vb = pop_back();
                Value va = pop_back();
                // numeric coercion
                bool aDouble = (va.type == ValueType::DOUBLE), bDouble = (vb.type == ValueType::DOUBLE);
                double da = (va.type == ValueType::DOUBLE)
                                ? va.current_value.d
                                : (va.type == ValueType::INT ? static_cast<double>(va.current_value.i) : 0.0);
                double db = (vb.type == ValueType::DOUBLE)
                                ? vb.current_value.d
                                : (vb.type == ValueType::INT ? static_cast<double>(vb.current_value.i) : 0.0);
                double res = 0;
                if (static_cast<Op>(op) == OP_ADD) res = da + db;
                if (static_cast<Op>(op) == OP_SUB) res = da - db;
                if (static_cast<Op>(op) == OP_MUL) res = da * db;
                if (static_cast<Op>(op) == OP_DIV) res = da / db;
                if (static_cast<Op>(op) == OP_MOD) res = fmod(da, db);
                // if both ints and not DIV -> int
                if (!aDouble && !bDouble && static_cast<Op>(op) != OP_DIV)
                    push_back(
                        Value::createINT(static_cast<int64_t>(res)));
                else push_back(Value::createDOUBLE(res));
                break;
            }

            // Logical
            case OP_AND: {
                Value vb = pop_back();
                Value va = pop_back();
                bool av = va.isTruthy();
                bool bv = vb.isTruthy();
                push_back(Value::createBOOL(av && bv));
                break;
            }
            case OP_OR: {
                Value vb = pop_back();
                Value va = pop_back();
                bool av = va.isTruthy();
                bool bv = vb.isTruthy();
                push_back(Value::createBOOL(av || bv));
                break;
            }
            case OP_NOT: {
                Value a = pop_back();
                push_back(Value::createBOOL(!a.isTruthy()));
                break;
            }

            // Comparison
            case OP_EQ:
            case OP_NEQ:
            case OP_LT:
            case OP_GT:
            case OP_LTE:
            case OP_GTE: {
                Value vb = pop_back();
                Value va = pop_back();
                bool res = false;
                // numeric or string or identity
                if ((va.type == ValueType::INT || va.type == ValueType::DOUBLE) && (
                        vb.type == ValueType::INT || vb.type == ValueType::DOUBLE)) {
                    double da = (va.type == ValueType::DOUBLE)
                                    ? va.current_value.d
                                    : static_cast<double>(va.current_value.i);
                    double db = (vb.type == ValueType::DOUBLE)
                                    ? vb.current_value.d
                                    : static_cast<double>(vb.current_value.i);
                    if (static_cast<Op>(op) == OP_EQ) res = (da == db);
                    if (static_cast<Op>(op) == OP_NEQ) res = (da != db);
                    if (static_cast<Op>(op) == OP_LT) res = (da < db);
                    if (static_cast<Op>(op) == OP_GT) res = (da > db);
                    if (static_cast<Op>(op) == OP_LTE) res = (da <= db);
                    if (static_cast<Op>(op) == OP_GTE) res = (da >= db);
                } else if (va.type == ValueType::OBJECT && vb.type == ValueType::OBJECT) {
                    // if both strings, compare strings
                    auto *sa = dynamic_cast<ObjString *>(va.current_value.object);
                    auto sb = dynamic_cast<ObjString *>(vb.current_value.object);
                    if (sa && sb) {
                        if (static_cast<Op>(op) == OP_EQ) res = (sa->value == sb->value);
                        if (static_cast<Op>(op) == OP_NEQ) res = (sa->value != sb->value);
                        if (static_cast<Op>(op) == OP_LT) res = (sa->value < sb->value);
                        if (static_cast<Op>(op) == OP_GT) res = (sa->value > sb->value);
                        if (static_cast<Op>(op) == OP_LTE) res = (sa->value <= sb->value);
                        if (static_cast<Op>(op) == OP_GTE) res = (sa->value >= sb->value);
                    } else {
                        // fallback identity
                        if (static_cast<Op>(op) == OP_EQ) res = (va.current_value.object == vb.current_value.object);
                        if (static_cast<Op>(op) == OP_NEQ) res = (va.current_value.object != vb.current_value.object);
                    }
                } else {
                    // fallback equality by type+value for int/double/bool
                    if (static_cast<Op>(op) == OP_EQ) res = (va.type == vb.type && va.toString() == vb.toString());
                    if (static_cast<Op>(op) == OP_NEQ) res = !(va.type == vb.type && va.toString() == vb.toString());
                }
                push_back(Value::createBOOL(res));
                break;
            }

            // Control flow
            case OP_JUMP: {
                uint32_t target = read_u32(frame);
                frame.ip = target;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint32_t target = read_u32(frame);
                Value cond = pop_back();
                if (!cond.isTruthy()) frame.ip = target;
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint32_t target = read_u32(frame);
                Value cond = pop_back();
                if (cond.isTruthy()) frame.ip = target;
                break;
            }
            case OP_RETURN: {
                uint8_t retCount = read_u8(frame);
                do_return(retCount);
                break;
            }
            case OP_CALL_NATIVE: {
                uint32_t nameIdx = read_u32(frame);
                uint8_t argc = read_u8(frame);

                if (nameIdx >= global_constants.size()) {
                    std::cerr << "CALL_NATIVE: bad nameIdx\n";
                    for (int i = 0; i < argc; i++) pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                Value nameVal = global_constants[nameIdx];

                auto *on = dynamic_cast<ObjString *>(nameVal.current_value.object);
                if (!on) {
                    std::cerr << "CALL_NATIVE: name not string\n";
                    for (int i = 0; i < argc; i++) pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                std::string nativeName = on->value;
                auto it = native_functions_registry.find(nativeName);
                if (it == native_functions_registry.end()) {
                    std::cerr << "CALL_NATIVE: not found " << nativeName << "\n";
                    for (int i = 0; i < argc; i++) pop_back();
                    push_back(Value::createNIL());
                } else {
                    it->second(*this, argc);
                }
                break;
            }
            case OP_CALL_FFI: {
                uint32_t libIdx = read_u32(frame);
                uint32_t symIdx = read_u32(frame);
                uint8_t argc = read_u8(frame);
                uint8_t sig = read_u8(frame);

                if (libIdx >= global_constants.size() || symIdx >= global_constants.size()) {
                    std::cerr << "CALL_FFI: bad idx\n";
                    for (int i = 0; i < argc; i++)
                        pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                auto *libNameObj = dynamic_cast<ObjString *>(global_constants[libIdx].current_value.object);
                auto *symNameObj = dynamic_cast<ObjString *>(global_constants[symIdx].current_value.object);

                if (!libNameObj || !symNameObj) {
                    std::cerr << "CALL_FFI: name types\n";
                    for (int i = 0; i < argc; i++)
                        pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                void *h = ffi.load_lib(libNameObj->value);
                if (!h) {
                    for (int i = 0; i < argc; i++)
                        pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                void *sym = ffi.find_symbol(h, symNameObj->value);
                if (!sym) {
                    std::cerr << "CALL_FFI: symbol missing\n";
                    for (int i = 0; i < argc; i++)
                        pop_back();
                    push_back(Value::createNIL());
                    break;
                }

                // Actually call the FFI function!
                // This is a simplified version - proper implementation needs libffi
                // For now, we support simple signatures based on 'sig' parameter

                std::vector<Value> args;
                for (int i = 0; i < argc; i++) {
                    args.insert(args.begin(), pop_back());
                }

                // Signature encoding:
                // 0 = int32(int32, int32)
                // 1 = int32(int32)
                // 2 = double(double, double)
                // etc.

                if (sig == 0 && argc == 2) {
                    // int32_t func(int32_t, int32_t)
                    typedef int32_t (*FFIFunc2Int)(int32_t, int32_t);
                    auto func = reinterpret_cast<FFIFunc2Int>(sym);

                    int32_t arg0 = (args[0].type == ValueType::INT)
                                       ? static_cast<int32_t>(args[0].current_value.i)
                                       : 0;
                    int32_t arg1 = (args[1].type == ValueType::INT)
                                       ? static_cast<int32_t>(args[1].current_value.i)
                                       : 0;

                    int32_t result = func(arg0, arg1);
                    push_back(Value::createINT(result));
                } else if (sig == 1 && argc == 1) {
                    // int32_t func(int32_t)
                    typedef int32_t (*FFIFunc1Int)(int32_t);
                    auto func = reinterpret_cast<FFIFunc1Int>(sym);

                    int32_t arg0 = (args[0].type == ValueType::INT)
                                       ? static_cast<int32_t>(args[0].current_value.i)
                                       : 0;

                    int32_t result = func(arg0);
                    push_back(Value::createINT(result));
                } else if (sig == 2 && argc == 2) {
                    // double func(double, double)
                    typedef double (*FFIFunc2Double)(double, double);
                    auto func = reinterpret_cast<FFIFunc2Double>(sym);

                    double arg0 = (args[0].type == ValueType::DOUBLE)
                                      ? args[0].current_value.d
                                      : (args[0].type == ValueType::INT
                                             ? static_cast<double>(args[0].current_value.i)
                                             : 0.0);
                    double arg1 = (args[1].type == ValueType::DOUBLE)
                                      ? args[1].current_value.d
                                      : (args[1].type == ValueType::INT
                                             ? static_cast<double>(args[1].current_value.i)
                                             : 0.0);

                    double result = func(arg0, arg1);
                    push_back(Value::createDOUBLE(result));
                } else {
                    std::cerr << "CALL_FFI: unsupported signature "
                            << static_cast<int>(sig) << "\n";
                    push_back(Value::createNIL());
                }

                break;
            }
            case OP_NEW_OBJECT: {
                uint32_t nameIdx = read_u32(frame);
                if (nameIdx >= global_constants.size()) {
                    push_back(Value::createNIL());
                    break;
                }
                auto *on = dynamic_cast<ObjString *>(global_constants[nameIdx].current_value.object);
                std::string className = on ? on->value : std::string("Object");
                ObjInstance *inst = allocate_instance(className);
                push_back(Value::createOBJECT(inst));
                break;
            }
            case OP_IS_INSTANCE: {
                uint32_t typeIdx = read_u32(frame);
                Value objVal = pop_back();

                if (typeIdx >= global_constants.size()) {
                    push_back(Value::createBOOL(false));
                    break;
                }

                auto* typeName = dynamic_cast<ObjString*>(global_constants[typeIdx].current_value.object);
                if (!typeName) {
                    push_back(Value::createBOOL(false));
                    break;
                }

                if (objVal.type != ValueType::OBJECT || !objVal.current_value.object) {
                    push_back(Value::createBOOL(false));
                    break;
                }

                auto* instance = dynamic_cast<ObjInstance*>(objVal.current_value.object);
                if (!instance) {
                    push_back(Value::createBOOL(false));
                    break;
                }

                // Check exact type match
                bool matches = (instance->className == typeName->value);

                push_back(Value::createBOOL(matches));
                break;
            }
            case OP_GET_FIELD: {
                uint32_t nameIdx = read_u32(frame);
                Value objv = pop_back();

                if (objv.type != ValueType::OBJECT || !objv.current_value.object) {
                    push_back(Value::createNIL());
                    break;
                }

                // Get the field name from constants
                if (nameIdx >= global_constants.size()) {
                    push_back(Value::createNIL());
                    break;
                }

                auto *on = dynamic_cast<ObjString *>(global_constants[nameIdx].current_value.object);
                if (!on) {
                    push_back(Value::createNIL());
                    break;
                }

                auto *oi = dynamic_cast<ObjInstance *>(objv.current_value.object);
                if (!oi) {
                    push_back(Value::createNIL());
                    break;
                }

                auto it = oi->fields.find(on->value);
                if (it == oi->fields.end()) {
                    push_back(Value::createNIL());
                } else {
                    push_back(it->second);
                }
                break;
            }
            case OP_SET_FIELD: {
                uint32_t nameIdx = read_u32(frame);
                Value val = pop_back();
                Value objv = pop_back();
                if (objv.type != ValueType::OBJECT || !objv.current_value.object) break;
                auto on = dynamic_cast<ObjString *>(global_constants[nameIdx].current_value.object);
                if (!on) break;
                auto *oi = dynamic_cast<ObjInstance *>(objv.current_value.object);
                if (!oi) break;
                oi->fields[on->value] = val;
                break;
            }

            // Array
            case OP_ARRAY_GET: {
                Value idxv = pop_back();
                Value arrv = pop_back();
                if (arrv.type != ValueType::OBJECT || !arrv.current_value.object) {
                    push_back(Value::createNIL());
                    break;
                }
                auto *oa = dynamic_cast<ObjArray *>(arrv.current_value.object);
                if (!oa) {
                    push_back(Value::createNIL());
                    break;
                }
                int idx = (idxv.type == ValueType::INT)
                              ? static_cast<int>(idxv.current_value.i)
                              : (idxv.type == ValueType::DOUBLE ? static_cast<int>(idxv.current_value.d) : 0);
                if (idx < 0 || idx >= static_cast<int>(oa->value.size())) push_back(Value::createNIL());
                else push_back(oa->value[idx]);
                break;
            }
            case OP_ARRAY_SET: {
                Value val = pop_back();
                Value idxv = pop_back();
                Value arrv = pop_back();
                if (arrv.type != ValueType::OBJECT || !arrv.current_value.object) break;
                auto oa = dynamic_cast<ObjArray *>(arrv.current_value.object);
                if (!oa) break;
                int idx = (idxv.type == ValueType::INT)
                              ? static_cast<int>(idxv.current_value.i)
                              : (idxv.type == ValueType::DOUBLE ? static_cast<int>(idxv.current_value.d) : 0);
                if (idx < 0) break;
                if (idx >= static_cast<int>(oa->value.size())) oa->value.resize(idx + 1, Value::createNIL());
                oa->value[idx] = val;
                break;
            }

            // Map
            case OP_MAP_SET: {
                Value val = pop_back();
                Value keyv = pop_back();
                Value mapv = pop_back();
                if (mapv.type != ValueType::OBJECT || !mapv.current_value.object) break;
                auto om = dynamic_cast<ObjMap *>(mapv.current_value.object);
                if (!om) break;
                std::string key = keyv.toString();
                om->value[key] = val;
                break;
            }
            case OP_MAP_GET: {
                Value keyv = pop_back();
                Value mapv = pop_back();
                if (mapv.type != ValueType::OBJECT || !mapv.current_value.object) {
                    push_back(Value::createNIL());
                    break;
                }
                auto om = dynamic_cast<ObjMap *>(mapv.current_value.object);
                if (!om) {
                    push_back(Value::createNIL());
                    break;
                }
                std::string key = keyv.toString();
                auto it = om->value.find(key);
                if (it == om->value.end()) push_back(Value::createNIL());
                else push_back(it->second);
                break;
            }

            // String ops
            case OP_STRING_CONCAT: {
                Value vb = pop_back();
                Value va = pop_back();
                std::string sa = (va.type == ValueType::OBJECT && va.current_value.object)
                                     ? dynamic_cast<ObjString *>(va.current_value.object)->value
                                     : va.toString();
                std::string sb = (vb.type == ValueType::OBJECT && vb.current_value.object)
                                     ? dynamic_cast<ObjString *>(vb.current_value.object)->value
                                     : vb.toString();
                ObjString *sNew = allocate_string(sa + sb);
                push_back(Value::createOBJECT(sNew));
                break;
            }
            case OP_STRING_LENGTH: {
                Value s = pop_back();
                if (s.type == ValueType::OBJECT && s.current_value.object) {
                    auto *os = dynamic_cast<ObjString *>(s.current_value.object);
                    if (os) {
                        push_back(Value::createINT(static_cast<int64_t>(os->value.size())));
                        break;
                    }
                }
                push_back(Value::createINT(0));
                break;
            }
            case OP_STRING_SUBSTR: {
                uint32_t start = read_u32(frame);
                uint32_t len = read_u32(frame);
                Value s = pop_back();
                if (s.type == ValueType::OBJECT && s.current_value.object) {
                    auto *os = dynamic_cast<ObjString *>(s.current_value.object);
                    if (os) {
                        uint32_t st = start;
                        if (st > os->value.size()) st = os->value.size();
                        uint32_t l = std::min<uint32_t>(len, static_cast<uint32_t>(os->value.size()) - st);
                        ObjString *out = allocate_string(os->value.substr(st, l));
                        push_back(Value::createOBJECT(out));
                        break;
                    }
                }
                push_back(Value::createOBJECT(allocate_string(std::string(""))));
                break;
            }
            case OP_STRING_EQ: {
                Value b = pop_back();
                Value a = pop_back();
                std::string sa = (a.type == ValueType::OBJECT && a.current_value.object)
                                     ? dynamic_cast<ObjString *>(a.current_value.object)->value
                                     : a.toString();
                std::string sb = (b.type == ValueType::OBJECT && b.current_value.object)
                                     ? dynamic_cast<ObjString *>(b.current_value.object)->value
                                     : b.toString();
                push_back(Value::createBOOL(sa == sb));
                break;
            }
            case OP_STRING_GET_CHAR: {
                Value idxv = pop_back();
                Value s = pop_back();
                int idx = (idxv.type == ValueType::INT) ? static_cast<int>(idxv.current_value.i) : 0;
                if (s.type == ValueType::OBJECT && s.current_value.object) {
                    auto os = dynamic_cast<ObjString *>(s.current_value.object);
                    if (os && idx >= 0 && idx < static_cast<int>(os->value.size())) {
                        std::string ch(1, os->value[idx]);
                        ObjString *out = allocate_string(ch);
                        push_back(Value::createOBJECT(out));
                        break;
                    }
                }
                push_back(Value::createOBJECT(allocate_string(std::string(""))));
                break;
            }

            // Globals
            case OP_LOAD_GLOBAL: {
                uint32_t nameIdx = read_u32(frame);
                if (nameIdx >= global_constants.size()) {
                    push_back(Value::createNIL());
                    break;
                }
                auto *on = dynamic_cast<ObjString *>(global_constants[nameIdx].current_value.object);
                if (!on) {
                    push_back(Value::createNIL());
                    break;
                }
                auto it = globals.find(on->value);
                if (it == globals.end()) push_back(Value::createNIL());
                else push_back(it->second);
                break;
            }
            case OP_STORE_GLOBAL: {
                uint32_t nameIdx = read_u32(frame);
                Value val = pop_back();
                if (nameIdx >= global_constants.size()) break;
                auto on = dynamic_cast<ObjString *>(global_constants[nameIdx].current_value.object);
                if (!on) break;
                globals[on->value] = val;
                break;
            }
            case OP_NEW_ARRAY: {
                ObjArray *arr = allocate_array();
                push_back(Value::createOBJECT(arr));
                break;
            }

            case OP_NEW_MAP: {
                ObjMap *map = allocate_map();
                push_back(Value::createOBJECT(map));
                break;
            }
            default:
                std::cerr << "Unimplemented opcode: " << static_cast<int>(op) << "\n";
                return;
        }
    }
}

uint32_t VM::add_global_string_constant(const std::string &s) {
    ObjString *os = allocate_string(s);
    const auto idx = static_cast<uint32_t>(global_constants.size());
    global_constants.push_back(Value::createOBJECT(os));
    return idx;
}
