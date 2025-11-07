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
 *  File: Object
 *  Created: 11/7/2025
 * ============================================================
 */
#ifndef DROPLET_OBJECT_H
#define DROPLET_OBJECT_H
#include <string>

// Object will be fully manager by the Garbage Collector
struct Object {
    virtual ~Object() = default;
    virtual std::string get_representor() const = 0;
};


#endif //DROPLET_OBJECT_H