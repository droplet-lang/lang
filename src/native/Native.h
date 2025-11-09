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
#ifndef DROPLET_NATIVE_H
#define DROPLET_NATIVE_H
#include "../vm/VM.h"

void native_len(VM& vm, const uint8_t argc);
void native_print(VM& vm, const uint8_t argc);
void native_println(VM& vm, const uint8_t argc);
void native_str(VM& vm, const uint8_t argc);

// made inline just to shut up compiler warning, no special case
inline void register_native_functions(VM& vm) {
    vm.register_native("print", native_print);
    vm.register_native("println", native_println);
    vm.register_native("str", native_str);
    vm.register_native("len", native_len);
}

#endif //DROPLET_NATIVE_H