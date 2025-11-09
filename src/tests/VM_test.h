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
 *  File: VM_test
 *  Created: 11/8/2025
 * ============================================================
 */
#ifndef DROPLET_VM_TEST_H
#define DROPLET_VM_TEST_H
#include <cmath>
#include <iostream>
#include <string>
#include "../vm/VM.h"
#include "../vm/DBC_helper.h"


// Helper to print test results
inline void printTestResult(const std::string& testName, const bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << "\n";
}

// Test 1: Basic arithmetic (2 + 3 = 5)
inline bool TEST_ARITHMETIC() {
    DBCBuilder builder;

    // Add constants
    uint32_t const2 = builder.addConstInt(2);
    uint32_t const3 = builder.addConstInt(3);

    // Create main function
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const2)      // push 2
        .pushConst(const3)      // push 3
        .emit(OP_ADD)           // add them
        .ret(1);                // return result

    // Write to file
    if (!builder.writeToFile("test_arithmetic.dbc")) {
        return false;
    }

    // Load and run
    VM vm;
    Loader loader;
    if (!loader.load_dbc_file("test_arithmetic.dbc", vm)) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    if (mainIdx == UINT32_MAX) {
        std::cerr << "main function not found\n";
        return false;
    }

    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    // Check result
    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 5;
}

// Test 2: Local variables
inline bool TEST_LOCAL_VAR() {
    DBCBuilder builder;

    uint32_t const10 = builder.addConstInt(10);
    uint32_t const20 = builder.addConstInt(20);

    // Function: local0 = 10, local1 = 20, return local0 + local1
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(2)
        .pushConst(const10)     // push 10
        .storeLocal(0)          // local0 = 10
        .pushConst(const20)     // push 20
        .storeLocal(1)          // local1 = 20
        .loadLocal(0)           // load local0
        .loadLocal(1)           // load local1
        .emit(OP_ADD)           // add
        .ret(1);

    if (!builder.writeToFile("test_locals.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_locals.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 30;
}

// Test 3: Comparison operations
inline bool TEST_CMP() {
    DBCBuilder builder;

    uint32_t const5 = builder.addConstInt(5);
    uint32_t const10 = builder.addConstInt(10);

    // Test: 5 < 10 (should be true)
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const5)
        .pushConst(const10)
        .emit(OP_LT)
        .ret(1);

    if (!builder.writeToFile("test_comparison.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_comparison.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::BOOL && result.current_value.b == true;
}

// Test 4: Conditional jump
inline bool TEST_COND_JMP() {
    DBCBuilder builder;

    uint32_t constTrue = builder.addConstBool(true);
    uint32_t const100 = builder.addConstInt(100);
    uint32_t const200 = builder.addConstInt(200);

    // if (true) return 100 else return 200
    auto& fn = builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(constTrue);

    uint32_t jumpPos = fn.currentPos();
    fn.jumpIfFalse(0);  // Will patch this

    // True branch
    fn.pushConst(const100)
        .ret(1);

    // False branch (patch jump target)
    uint32_t falsePos = fn.currentPos();
    fn.pushConst(const200)
        .ret(1);

    // Manually patch the jump (in real use, you'd track this)
    // For this test, we know true branch executes

    if (!builder.writeToFile("test_jump.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_jump.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 100;
}

// Test 5: String operations
inline bool TEST_STR_OPS() {
    DBCBuilder builder;

    uint32_t strHello = builder.addConstString("Hello");
    uint32_t strWorld = builder.addConstString("World");

    // Concatenate "Hello" + "World"
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(strHello)
        .pushConst(strWorld)
        .emit(OP_STRING_CONCAT)
        .ret(1);

    if (!builder.writeToFile("test_string.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_string.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    if (result.type != ValueType::OBJECT || !result.current_value.object) {
        return false;
    }

    auto* str = dynamic_cast<ObjString*>(result.current_value.object);
    return str && str->value == "HelloWorld";
}

// Test 6: Global variables
inline bool TEST_GLOBALS() {
    DBCBuilder builder;

    uint32_t varName = builder.addConstString("myGlobal");
    uint32_t const42 = builder.addConstInt(42);

    // Store 42 to global "myGlobal" and load it back
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const42)
        .storeGlobal(varName)
        .loadGlobal(varName)
        .ret(1);

    if (!builder.writeToFile("test_globals.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_globals.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 42;
}

// Test 7: Function calls
inline bool TEST_FN_CALLS() {
    DBCBuilder builder;

    uint32_t const5 = builder.addConstInt(5);
    uint32_t const3 = builder.addConstInt(3);

    // Helper function: add(a, b) returns a + b
    builder.addFunction("add")
        .setArgCount(2)
        .setLocalCount(0)
        .loadLocal(0)    // arg0
        .loadLocal(1)    // arg1
        .emit(OP_ADD)
        .ret(1);

    // Main function: call add(5, 3)
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const5)
        .pushConst(const3)
        .call(0, 2)      // call function index 0 with 2 args
        .ret(1);

    if (!builder.writeToFile("test_calls.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_calls.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 8;
}

// Test 8: Array operations
inline bool TEST_ARRAY_OPS() {
    DBCBuilder builder;

    uint32_t const0 = builder.addConstInt(0);
    uint32_t const100 = builder.addConstInt(100);

    // Create array, set arr[0] = 100, return arr[0]
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(1)
        .newArray()              // Create array
        .storeLocal(0)           // Store in local0
        .loadLocal(0)            // Load array
        .pushConst(const0)       // Index 0
        .pushConst(const100)     // Value 100
        .arraySet()              // arr[0] = 100
        .loadLocal(0)            // Load array
        .pushConst(const0)       // Index 0
        .arrayGet()              // Get arr[0]
        .ret(1);

    if (!builder.writeToFile("test_array.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_array.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 100;
}

// Test 9: Map operations
inline bool TEST_MAP_OPS() {
    DBCBuilder builder;

    uint32_t keyStr = builder.addConstString("myKey");
    uint32_t const42 = builder.addConstInt(42);

    // Create map, set map["myKey"] = 42, return map["myKey"]
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(1)
        .newMap()                // Create map
        .storeLocal(0)           // Store in local0
        .loadLocal(0)            // Load map
        .pushConst(keyStr)       // Key "myKey"
        .pushConst(const42)      // Value 42
        .mapSet()                // map["myKey"] = 42
        .loadLocal(0)            // Load map
        .pushConst(keyStr)       // Key "myKey"
        .mapGet()                // Get map["myKey"]
        .ret(1);

    if (!builder.writeToFile("test_map.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_map.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 42;
}

// Test 9: Double arithmetic
inline bool TEST_DOUBLE_ARITHMETIC() {
    DBCBuilder builder;

    uint32_t const2_5 = builder.addConstDouble(2.5);
    uint32_t const3_5 = builder.addConstDouble(3.5);

    // Test: 2.5 + 3.5 = 6.0
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const2_5)
        .pushConst(const3_5)
        .emit(OP_ADD)
        .ret(1);

    if (!builder.writeToFile("test_double.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_double.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::DOUBLE && result.current_value.d == 6.0;
}

// Test 10: Object field operations
inline bool TEST_OBJ_FIELD_OPS() {
    DBCBuilder builder;

    uint32_t objName = builder.addConstString("TestObj");
    uint32_t fieldName = builder.addConstString("value");
    uint32_t const42 = builder.addConstInt(42);

    // Create object, set field "value" = 42, get field "value"
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(1)
        .newObject(objName)      // Create object
        .storeLocal(0)           // Store in local0
        .loadLocal(0)            // Load object
        .pushConst(const42)      // Value 42
        .setField(fieldName)     // obj.value = 42
        .loadLocal(0)            // Load object
        .getField(fieldName)     // Get obj.value
        .ret(1);

    if (!builder.writeToFile("test_object.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_object.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 42;
}

// Test 11: Native function call - Simple math operation
inline bool TEST_NATIVE_CALL_SIMPLE() {
    DBCBuilder builder;

    // Register a native function that squares a number
    auto nativeSquare = [](VM& vm, int argc) {
        if (argc != 1) {
            vm.push_back(Value::createNIL());
            return;
        }
        Value arg = vm.pop_back();
        if (arg.type == ValueType::INT) {
            int64_t val = arg.current_value.i;
            vm.push_back(Value::createINT(val * val));
        } else if (arg.type == ValueType::DOUBLE) {
            double val = arg.current_value.d;
            vm.push_back(Value::createDOUBLE(val * val));
        } else {
            vm.push_back(Value::createNIL());
        }
    };

    uint32_t funcName = builder.addConstString("square");
    uint32_t const5 = builder.addConstInt(5);

    // Main function: call native square(5), should return 25
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const5)           // Push argument 5
        .emit(OP_CALL_NATIVE)        // Call native function
        .emitU32(funcName)           // Function name index
        .emitU8(1)                   // 1 argument
        .ret(1);

    if (!builder.writeToFile("test_native_simple.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("square", nativeSquare);

    if (!vm.load_dbc_file("test_native_simple.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 25;
}

// Test 12: Native function call - String manipulation
inline bool TEST_NATIVE_CALL_STRING() {
    DBCBuilder builder;

    // Register a native function that converts string to uppercase
    auto nativeUppercase = [](VM& vm, int argc) {
        if (argc != 1) {
            vm.push_back(Value::createNIL());
            return;
        }
        Value arg = vm.pop_back();
        if (arg.type == ValueType::OBJECT && arg.current_value.object) {
            auto* str = dynamic_cast<ObjString*>(arg.current_value.object);
            if (str) {
                std::string upper = str->value;
                for (char& c : upper) {
                    c = std::toupper(c);
                }
                ObjString* newStr = vm.allocate_string(upper);
                vm.push_back(Value::createOBJECT(newStr));
                return;
            }
        }
        vm.push_back(Value::createNIL());
    };

    uint32_t funcName = builder.addConstString("uppercase");
    uint32_t testStr = builder.addConstString("hello");

    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(testStr)          // Push "hello"
        .emit(OP_CALL_NATIVE)        // Call native function
        .emitU32(funcName)           // Function name index
        .emitU8(1)                   // 1 argument
        .ret(1);

    if (!builder.writeToFile("test_native_string.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("uppercase", nativeUppercase);

    if (!vm.load_dbc_file("test_native_string.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    if (result.type != ValueType::OBJECT || !result.current_value.object) {
        return false;
    }
    auto* str = dynamic_cast<ObjString*>(result.current_value.object);
    return str && str->value == "HELLO";
}

// Test 13: Native function call - Multiple arguments
inline bool TEST_NATIVE_CALL_MULTI_ARG() {
    DBCBuilder builder;

    // Register a native function that adds three numbers
    auto nativeAddThree = [](VM& vm, int argc) {
        if (argc != 3) {
            vm.push_back(Value::createNIL());
            return;
        }
        Value c = vm.pop_back();
        Value b = vm.pop_back();
        Value a = vm.pop_back();

        int64_t sum = 0;
        if (a.type == ValueType::INT) sum += a.current_value.i;
        if (b.type == ValueType::INT) sum += b.current_value.i;
        if (c.type == ValueType::INT) sum += c.current_value.i;

        vm.push_back(Value::createINT(sum));
    };

    uint32_t funcName = builder.addConstString("addThree");
    uint32_t const10 = builder.addConstInt(10);
    uint32_t const20 = builder.addConstInt(20);
    uint32_t const30 = builder.addConstInt(30);

    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const10)          // Push 10
        .pushConst(const20)          // Push 20
        .pushConst(const30)          // Push 30
        .emit(OP_CALL_NATIVE)        // Call native function
        .emitU32(funcName)           // Function name index
        .emitU8(3)                   // 3 arguments
        .ret(1);

    if (!builder.writeToFile("test_native_multiarg.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("addThree", nativeAddThree);

    if (!vm.load_dbc_file("test_native_multiarg.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 60;
}

// Test 14: Native function - Math operations (sin, cos)
inline bool TEST_NATIVE_CALL_MATH() {
    DBCBuilder builder;

    // Register sine function
    auto nativeSin = [](VM& vm, int argc) {
        if (argc != 1) {
            vm.push_back(Value::createNIL());
            return;
        }
        Value arg = vm.pop_back();
        double val = 0.0;
        if (arg.type == ValueType::DOUBLE) {
            val = arg.current_value.d;
        } else if (arg.type == ValueType::INT) {
            val = static_cast<double>(arg.current_value.i);
        }
        vm.push_back(Value::createDOUBLE(std::sin(val)));
    };

    uint32_t funcName = builder.addConstString("sin");
    uint32_t constPi = builder.addConstDouble(3.14159265359);

    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(constPi)          // Push PI
        .emit(OP_CALL_NATIVE)        // Call sin(PI)
        .emitU32(funcName)
        .emitU8(1)
        .ret(1);

    if (!builder.writeToFile("test_native_math.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("sin", nativeSin);

    if (!vm.load_dbc_file("test_native_math.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    // sin(PI) should be very close to 0
    return result.type == ValueType::DOUBLE && std::abs(result.current_value.d) < 0.0001;
}

// Test 15: Native function - Array manipulation
inline bool TEST_NATIVE_ARRAY_OPS() {
    DBCBuilder builder;

    // Native function that returns the sum of array elements
    auto nativeArraySum = [](VM& vm, int argc) {
        if (argc != 1) {
            vm.push_back(Value::createNIL());
            return;
        }
        Value arrVal = vm.pop_back();
        if (arrVal.type != ValueType::OBJECT || !arrVal.current_value.object) {
            vm.push_back(Value::createINT(0));
            return;
        }
        auto* arr = dynamic_cast<ObjArray*>(arrVal.current_value.object);
        if (!arr) {
            vm.push_back(Value::createINT(0));
            return;
        }

        int64_t sum = 0;
        for (const auto& val : arr->value) {
            if (val.type == ValueType::INT) {
                sum += val.current_value.i;
            }
        }
        vm.push_back(Value::createINT(sum));
    };

    uint32_t funcName = builder.addConstString("arraySum");
    uint32_t const0 = builder.addConstInt(0);
    uint32_t const1 = builder.addConstInt(1);
    uint32_t const2 = builder.addConstInt(2);
    uint32_t const10 = builder.addConstInt(10);
    uint32_t const20 = builder.addConstInt(20);
    uint32_t const30 = builder.addConstInt(30);

    // Create array [10, 20, 30] and sum it
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(1)
        .newArray()                  // Create array
        .storeLocal(0)               // Store in local0
        // Set arr[0] = 10
        .loadLocal(0)
        .pushConst(const0)
        .pushConst(const10)
        .arraySet()
        // Set arr[1] = 20
        .loadLocal(0)
        .pushConst(const1)
        .pushConst(const20)
        .arraySet()
        // Set arr[2] = 30
        .loadLocal(0)
        .pushConst(const2)
        .pushConst(const30)
        .arraySet()
        // Call native sum function
        .loadLocal(0)
        .emit(OP_CALL_NATIVE)
        .emitU32(funcName)
        .emitU8(1)
        .ret(1);

    if (!builder.writeToFile("test_native_array.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("arraySum", nativeArraySum);

    if (!vm.load_dbc_file("test_native_array.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    return result.type == ValueType::INT && result.current_value.i == 60;
}

// Test 16: Native function error handling - Wrong argument count
inline bool TEST_NATIVE_ERROR_HANDLING() {
    DBCBuilder builder;

    // Native function that expects 2 args but will receive 1
    auto nativeAdd = [](VM& vm, int argc) {
        if (argc != 2) {
            std::cerr << "Expected 2 arguments, got " << argc << "\n";
            // Pop all arguments
            for (int i = 0; i < argc; i++) vm.pop_back();
            vm.push_back(Value::createNIL());
            return;
        }
        Value b = vm.pop_back();
        Value a = vm.pop_back();
        int64_t sum = 0;
        if (a.type == ValueType::INT) sum += a.current_value.i;
        if (b.type == ValueType::INT) sum += b.current_value.i;
        vm.push_back(Value::createINT(sum));
    };

    uint32_t funcName = builder.addConstString("add");
    uint32_t const5 = builder.addConstInt(5);

    // Call with only 1 argument instead of 2
    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const5)           // Push only 1 arg
        .emit(OP_CALL_NATIVE)
        .emitU32(funcName)
        .emitU8(1)                   // Wrong arg count
        .ret(1);

    if (!builder.writeToFile("test_native_error.dbc")) {
        return false;
    }

    VM vm;
    vm.register_native("add", nativeAdd);

    if (!vm.load_dbc_file("test_native_error.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    // Should return NIL due to wrong argument count
    return result.type == ValueType::NIL;
}

inline bool TEST_FFI_WORKING() {
    // First, create and compile the test library
    std::cout << "Creating FFI test library...\n";

    // Write test library source
    std::ofstream libSource("ffi_test.c");
    libSource << R"(
#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

EXPORT int add_int(int a, int b) {
    return a + b;
}

EXPORT int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
)";
    libSource.close();

    // Compile it
#ifdef _WIN32
    system("gcc -shared -o ffi_test.dll ffi_test.c");
    std::string libPath = "ffi_test.dll";
#else
    system("gcc -shared -fPIC -o ffi_test.so ffi_test.c");
    std::string libPath = "./ffi_test.so";
#endif

    DBCBuilder builder;

    uint32_t libPathIdx = builder.addConstString(libPath);
    uint32_t addSymbol = builder.addConstString("add_int");
    uint32_t const5 = builder.addConstInt(5);
    uint32_t const7 = builder.addConstInt(7);

    builder.addFunction("main")
        .setArgCount(0)
        .setLocalCount(0)
        .pushConst(const5)
        .pushConst(const7)
        .callFFI(libPathIdx, addSymbol, 2, 0)  // sig=0 for int(int,int)
        .ret(1);

    if (!builder.writeToFile("test_ffi_working.dbc")) {
        return false;
    }

    VM vm;
    if (!vm.load_dbc_file("test_ffi_working.dbc")) {
        return false;
    }

    uint32_t mainIdx = vm.get_function_index("main");
    vm.call_function_by_index(mainIdx, 0);
    vm.run();

    Value result = vm.peek_back(0);
    bool success = result.type == ValueType::INT && result.current_value.i == 12;

    // Cleanup
    std::remove("ffi_test.c");
#ifdef _WIN32
    std::remove("ffi_test.dll");
#else
    std::remove("ffi_test.so");
#endif

    return success;
}

inline int ALL_TESTS_VM() {
    int passed = 0;
    int total = 0;

    auto runTest = [&](const std::string& name, bool (*test)()) {
        total++;
        bool result = test();
        printTestResult(name, result);
        if (result) passed++;
    };

    runTest("Arithmetic Operations", TEST_ARITHMETIC);
    runTest("Local Variables", TEST_LOCAL_VAR);
    runTest("Comparison Operations", TEST_CMP);
    runTest("Conditional Jumps", TEST_COND_JMP);
    runTest("String Operations", TEST_STR_OPS);
    runTest("Global Variables", TEST_GLOBALS);
    runTest("Function Calls", TEST_FN_CALLS);
    runTest("Array Operations", TEST_ARRAY_OPS);
    runTest("Map Operations", TEST_MAP_OPS);
    runTest("Object Field Operations", TEST_OBJ_FIELD_OPS);
    runTest("Double Arithmetic", TEST_DOUBLE_ARITHMETIC);
    runTest("Native Call - Simple Math", TEST_NATIVE_CALL_SIMPLE);
    runTest("Native Call - String Manipulation", TEST_NATIVE_CALL_STRING);
    runTest("Native Call - Multiple Arguments", TEST_NATIVE_CALL_MULTI_ARG);
    runTest("Native Call - Math Operations", TEST_NATIVE_CALL_MATH);
    runTest("Native Call - Array Operations", TEST_NATIVE_ARRAY_OPS);
    runTest("Native Call - Error Handling", TEST_NATIVE_ERROR_HANDLING);
    runTest("FFI ", TEST_FFI_WORKING);

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << passed << "/" << total << "\n";

    return (passed == total) ? 0 : 1;
}
#endif //DROPLET_VM_TEST_H