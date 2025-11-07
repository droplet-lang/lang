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
#include <unordered_map>
#include <vector>

// Object will be fully manager by the Garbage Collector
struct Object {
    virtual ~Object() = default;
    virtual std::string get_representor() const = 0;
};

/*
 * There will be primarily support for
 * Array, String, Map and Object Instance
 * I think this should handle all of the type of values that we can handle
 *
 * Normal primitive value will be handled as normal values (see Value) and other will be handled as Object
*/
struct ObjString final : Object {
    std::string value;

    explicit ObjString(const std::string &v): value(v) {}

    std::string get_representor() const override {
        // "hello"
        return "\"" + value + "\"";
    }
};

struct ObjArray final: Object {
    // will be easy to manage, can even get len, cap later lol
    std::vector<Value> value;

    std::string get_representor() const override {
        // <array>
        return "<array>";
    }
};

struct ObjMap final: Object {
    // Todo: (@svpz) change to hash based implementation
    // for initial implementation, lets treat key as string
    // this will be easy for us now
    // later we can rely on hash based implementation
    std::unordered_map<std::string, Value> value;

    std::string get_representor() const override {
        return "<map>";
    }
};

struct ObjInstance final: Object {
    std::string className;
    std::unordered_map<std::string, Value> fields;

    explicit ObjInstance(const std::string &cn="Object"): className(cn) {}

    std::string get_representor() const override {
        // <object:HelloWorld>
        return "<object:" + className + ">";
    }
};

#endif //DROPLET_OBJECT_H