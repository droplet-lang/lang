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
#include <iostream>

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

    // collect return values (supports only 1 for simplicity)
    std::vector<Value> rets;

    for (int i = 0; i < return_count; i++)
        rets.push_back(pop_back());

    const CallFrame frame = call_frames.back();
    call_frames.pop_back();

    // shrink stack to slotStart (drop locals and args)
    const uint32_t restore = frame.localStartsAt;

    while (sp_internal > restore)
        pop_back();

    // push return values back in original order
    for (int i = static_cast<int>(rets.size()) - 1; i >= 0; i--)
        push_back(rets[i]);
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
        for (const auto &val: globals | std::views::values) mark(val);
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
}

void VM::run() {
    // GC GC GC
    auto rootWalker = [this](const MarkerFunction &mark) {
        for (uint32_t i = 0; i < sp_internal; i++) {
            mark(stack[i]);
        }

        for (const auto &val: globals | std::views::values) {
            mark(val);
        }
    };

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
            case OP_LOAD_LOCAL: {
                uint8_t slot = read_u8(frame);
                uint32_t abs = frame.localStartsAt + slot;
                if (abs < sp_internal) {
                    push_back(stack_internal[abs]);
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
                double da = (va.type == ValueType::DOUBLE) ? va.current_value.d : (va.type == ValueType::INT ? static_cast<double>(va.current_value.i) : 0.0);
                double db = (vb.type == ValueType::DOUBLE) ? vb.current_value.d : (vb.type == ValueType::INT ? static_cast<double>(vb.current_value.i) : 0.0);
                double res = 0;
                if (static_cast<Op>(op) == OP_ADD) res = da + db;
                if (static_cast<Op>(op) == OP_SUB) res = da - db;
                if (static_cast<Op>(op) == OP_MUL) res = da * db;
                if (static_cast<Op>(op) == OP_DIV) res = da / db;
                if (static_cast<Op>(op) == OP_MOD) res = fmod(da, db);
                // if both ints and not DIV -> int
                if (!aDouble && !bDouble && static_cast<Op>(op) != OP_DIV) push_back(Value::createINT(static_cast<int64_t>(res)));
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
                    double da = (va.type == ValueType::DOUBLE) ? va.current_value.d : static_cast<double>(va.current_value.i);
                    double db = (vb.type == ValueType::DOUBLE) ? vb.current_value.d : static_cast<double>(vb.current_value.i);
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
                        if (static_cast<Op>(op) == OP_LT) res = (sa->value< sb->value);
                        if (static_cast<Op>(op) == OP_GT) res = (sa->value> sb->value);
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

            case OP_CALL:
                break;
            case OP_RETURN:
                break;
            case OP_CALL_NATIVE:
                break;
            case OP_CALL_FFI:
                break;
            case OP_NEW_OBJECT:
                break;
            case OP_GET_FIELD:
                break;
            case OP_SET_FIELD:
                break;
            case OP_ARRAY_GET:
                break;
            case OP_ARRAY_SET:
                break;
            case OP_MAP_SET:
                break;
            case OP_MAP_GET:
                break;
            case OP_STRING_CONCAT:
                break;
            case OP_STRING_LENGTH:
                break;
            case OP_STRING_SUBSTR:
                break;
            case OP_STRING_EQ:
                break;
            case OP_STRING_GET_CHAR:
                break;
            case OP_LOAD_GLOBAL:
                break;
            case OP_STORE_GLOBAL:
                break;
        }
    }
}
