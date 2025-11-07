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
 *  File: Value
 *  Created: 11/7/2025
 * ============================================================
 */
#include "Value.h"
#include "Object.h"

Value Value::createNIL() {
    return {};
}

Value Value::createBOOL(const bool v) {
    Value vo;
    vo.type = ValueType::BOOL;
    vo.current_value.b = v;
    return vo;
}

Value Value::createINT(const int v) {
    Value vo;
    vo.type = ValueType::INT;
    vo.current_value.i = v;
    return vo;
}

Value Value::createDOUBLE(const double v) {
    Value vo;
    vo.type = ValueType::DOUBLE;
    vo.current_value.d = v;
    return vo;
}

Value Value::createOBJECT(Object* v) {
    Value vo;
    vo.type = ValueType::OBJECT;
    vo.current_value.object = v;
    return vo;
}

std::string Value::toString() const {
    switch(type) {
        case ValueType::NIL: return "nil";
        case ValueType::BOOL: return current_value.b ? "true" : "false";
        case ValueType::INT: return std::to_string(current_value.i);
        case ValueType::DOUBLE: return std::to_string(current_value.d);
        case ValueType::OBJECT:
            if (!current_value.object)
                return "nilobj";
            return current_value.object->get_representor();
    }
    return "????";
}