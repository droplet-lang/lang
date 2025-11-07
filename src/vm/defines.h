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
 *  File: defines
 *  Created: 11/7/2025
 * ============================================================
 */
#ifndef DROPLET_DEFINES_H
#define DROPLET_DEFINES_H
#include <functional>

#include "Value.h"

typedef std::function<void(Value)> MarkerFunction;
typedef std::function<void(MarkerFunction)> RWComplexGCFunction;

#endif //DROPLET_DEFINES_H