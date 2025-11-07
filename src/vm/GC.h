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
 *  File: GC
 *  Created: 11/7/2025
 * ============================================================
 */
#ifndef DROPLET_GC_H
#define DROPLET_GC_H
#include <functional>
#include <vector>

#include "defines.h"
#include "Object.h"
#include "Value.h"

#ifndef MEM_THRESHOLD_FOR_NEXT_GC_CALL
    // Todo (@svpz) We can even let user fine tune this???
    #define MEM_THRESHOLD_FOR_NEXT_GC_CALL 1024 * 1024
#endif


// For current implementation, we will use normal mark-sweep
// based Garbage Collector, refinement is always welcome
// ref https://www.geeksforgeeks.org/java/mark-and-sweep-garbage-collection-algorithm/
struct GC {
    std::vector<Object*> heap;
    size_t memThresholdForNextGcCall = MEM_THRESHOLD_FOR_NEXT_GC_CALL;

    void allocNewObject(Object *obj);
    void markAll(const RWComplexGCFunction &rootWalker);
    void markValue(Value value);
    void sweep();
    void collectIfNeeded(const RWComplexGCFunction &rootWalker);
    void collect(const RWComplexGCFunction &rootWalker);
};


#endif //DROPLET_GC_H