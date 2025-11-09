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
 *  File: FFI
 *  Created: 11/9/2025
 * ============================================================
 */
#include "FFI.h"

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