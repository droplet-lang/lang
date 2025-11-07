# DLVM - Design Spec
1. Stack Based
2. Supports Dynamic Dispatch
3. Builtin Types
4. Native Function Integration
5. FFI
6. Runtime Error Handling

![]("assets/dlvm.png")

## Bytecodes
```
Stack Ops: PUSH_CONST, POP, LOAD_LOCAL, STORE_LOCAL, DUP, SWAP, ROT

Arithmetic: ADD, SUB, MUL, DIV, MOD

Logical: AND, OR, NOT

Comparison: EQ, NEQ, LT, GT, LTE, GTE

Control Flow: JUMP, JUMP_IF_FALSE, JUMP_IF_TRUE

Function Calls: CALL, RETURN, CALL_NATIVE, CALL_FFI

Object Ops: NEW_OBJECT, GET_FIELD, SET_FIELD

Array Ops: ARRAY_GET, ARRAY_SET

Map Ops: AMP_SET, MAP_GET

String Ops: STRING_CONCAT, STRING_LENGTH, STRING_SUBSTR, STRING_EQ, STRING_GET_CHAR

Err Handling: TRY, CATCH, THROW or go like??? (TODO: @svpz), if yes RETURN should support args
```

### Usecase

```
PUSH_CONST 10   // stack: [10]
PUSH_CONST "Hi" // stack: [10, "Hi"]

PUSH_CONST 5
POP            // stack: []

local[0] = 42
LOAD_LOCAL 0   // stack: [42]

PUSH_CONST 100
STORE_LOCAL 0  // local[0] = 100

JUMP addr   // moves IP to addr

PUSH_CONST false
JUMP_IF_FALSE addr  // jumps because top of stack is false

// function at addr 100
CALL 100, 2  // calls function with 2 arguments on stack

RETURN 1 // pops return value from stack and resumes caller

CALL_NATIVE "print", 1  // pops 1 argument from stack and calls nativePrint

CALL_FFI "math.so", "my_sqrt", 1  // pops 1 argument, pushes return value

NEW_OBJECT ClassID   // pushes object reference onto stack

PUSH obj_ref
GET_FIELD "x"   // stack: [obj_ref.x]

PUSH obj_ref
PUSH_CONST 10
SET_FIELD "x"  // obj_ref.x = 10

PUSH arr_ref
PUSH_CONST 2
ARRAY_GET   // stack: [arr[2]]

PUSH arr_ref
PUSH_CONST 2
PUSH_CONST 99
ARRAY_SET   // arr[2] = 99
```

### Example
```
class HelloWorld {
    int x;

    HelloWorld(int initial) {
        x = initial;
    }

    int add(int a, int b) {
        return a + b;
    }
}

class Calculator {
    int add(int x, int y) {
        return x * y; 
    }
}

// Usage
var obj1 = HelloWorld(5);
var obj2 = Calculator();
var result1 = obj1.add(10, 20); // HelloWorld.add
var result2 = obj2.add(10, 20); // Calculator.add
print(result1);
print(result2);
```

```
# -----------------------------
# Global Execution
# -----------------------------
# obj1 = HelloWorld(5)
NEW_OBJECT HelloWorld
PUSH_CONST 5
CALL HelloWorld.init, 2
STORE_LOCAL 0

# obj2 = Calculator()
NEW_OBJECT Calculator
CALL Calculator.init, 1
STORE_LOCAL 1

# result1 = obj1.add(10,20) -> HelloWorld.add_int_int
LOAD_LOCAL 0
PUSH_CONST 10
PUSH_CONST 20
CALL HelloWorld.add_int_int, 3
STORE_LOCAL 2

# result2 = obj2.add(10,20) -> Calculator.add_int_int
LOAD_LOCAL 1
PUSH_CONST 10
PUSH_CONST 20
CALL Calculator.add_int_int, 3
STORE_LOCAL 3

# print results
LOAD_LOCAL 2
CALL_NATIVE "print", 1

LOAD_LOCAL 3
CALL_NATIVE "print", 1

# -----------------------------
# Function Definitions
# -----------------------------

# HelloWorld.init(self, initial)
HelloWorld.init:
LOAD_LOCAL 0
LOAD_LOCAL 1
SET_FIELD "x"
RETURN

# HelloWorld.add_int_int(self, a, b)
HelloWorld.add_int_int:
LOAD_LOCAL 1
LOAD_LOCAL 2
ADD
RETURN

# HelloWorld.add_double_double(self, a, b)
HelloWorld.add_double_double:
LOAD_LOCAL 1
LOAD_LOCAL 2
ADD
RETURN

# Calculator.init(self)
Calculator.init:
RETURN

# Calculator.add_int_int(self, x, y)
Calculator.add_int_int:
LOAD_LOCAL 1
LOAD_LOCAL 2
MUL
RETURN
```