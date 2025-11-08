# Droplet Language Design Document

## 1. Overview

Droplet is a statically-typed, object-oriented language that compiles to bytecode for execution on a stack-based VM. This document specifies features implementable with the current VM architecture without requiring VM modifications.

---

## 2. Type System

### 2.1 Primitive Types
```droplet
int      // 64-bit signed integer (VM: ValueType::INT)
float    // 64-bit double (VM: ValueType::DOUBLE)
bool     // Boolean (VM: ValueType::BOOL)
str      // String (VM: ObjString)
null     // Nil value (VM: ValueType::NIL)
```

### 2.2 Composite Types
```droplet
list[T]      // Dynamic array (VM: ObjArray)
dict[K,V]    // Hash map (VM: ObjMap)
obj          // Base object type (VM: ObjInstance)
```

### 2.3 Generics (Compile-Time Monomorphization)
```droplet
class Box[T] {
    value: T
    fn get() -> T { ret value }
}

// Compiler generates separate implementations:
// Box_int, Box_str, etc.
let intBox = Box[int](42)
let strBox = Box[str]("hello")
```

**Implementation:** Compiler creates separate class definitions for each type parameter instantiation.

---

## 3. Classes and Objects

### 3.1 Basic Class Definition
```droplet
class Point {
    x: float
    y: float
    
    new(x: float, y: float) {
        self.x = x
        self.y = y
    }
    
    fn distance() -> float {
        ret (self.x * self.x + self.y * self.y) ** 0.5
    }
}
```

**Bytecode Mapping:**
- `new()` → Function that calls `OP_NEW_OBJECT`, then `OP_SET_FIELD` for each field
- Method calls → `OP_CALL` with implicit `self` as first argument
- Field access → `OP_GET_FIELD` / `OP_SET_FIELD`

### 3.2 Multiple Constructors via Named Constructors
```droplet
class Point {
    x: float
    y: float
    
    // Primary constructor
    new(x: float, y: float) {
        self.x = x
        self.y = y
    }
    
    // Named constructors (static factory methods)
    static fn fromX(x: float) -> Point {
        ret Point(x, 0.0)
    }
    
    static fn origin() -> Point {
        ret Point(0.0, 0.0)
    }
}

// Usage:
let p1 = Point(3.0, 4.0)
let p2 = Point.fromX(5.0)
let p3 = Point.origin()
```

### 3.3 Static Members
```droplet
class Counter {
    static count: int = 0
    id: int
    
    new() {
        self.id = Counter.count
        Counter.count = Counter.count + 1
    }
    
    static fn reset() {
        Counter.count = 0
    }
}
```

**Implementation:**
- Static fields → Global variables with mangled names (`Counter$$count`)
- Static methods → Top-level functions with mangled names
- Access through class name → Compiler resolves to global reference

---

## 4. Inheritance

### 4.1 Single Inheritance
```droplet
class Animal {
    name: str
    
    new(name: str) {
        self.name = name
    }
    
    fn speak() -> str {
        ret "..."
    }
}

class Dog : Animal {
    breed: str
    
    new(name: str, breed: str) {
        Animal.new(name)  // Explicit parent constructor
        self.breed = breed
    }
    
    fn speak() -> str {
        ret "Woof!"
    }
}
```

**Implementation Strategy:**
1. **Field Layout:** Child class contains all parent fields first
2. **Method Resolution:** Compiler builds method dispatch table
3. **Constructor Chaining:** Parent constructor called via explicit `OP_CALL`
4. **Type Checking:** Store class hierarchy in compiler metadata

```
// Compiled field layout for Dog instance:
// [name (from Animal), breed (from Dog)]

// Method dispatch table:
// Dog.speak -> Dog$$speak function
// Dog.name (inherited field) -> offset 0
```

### 4.2 Parent Method Access
```droplet
class Child : Parent {
    fn method() -> int {
        let parentResult = Parent.method()  // Call parent implementation
        ret parentResult + 10
    }
}
```

**Implementation:** Compiler resolves `Parent.method()` to specific function index in parent class.

---

## 5. Access Modifiers (Compile-Time Only)

```droplet
class BankAccount {
    pub balance: float       // Accessible everywhere
    prot accountId: str      // Accessible in subclasses
    priv pin: int           // Only within this class
    
    pub fn deposit(amount: float) {
        self.balance = self.balance + amount
    }
    
    priv fn validatePin(input: int) -> bool {
        ret input == self.pin
    }
}
```

**Implementation:**
- Compiler enforces visibility rules during compilation
- No runtime checks needed
- All fields stored normally in VM; security is compile-time

---

## 6. Sealed Classes and Methods

```droplet
seal class FinalClass {  // Cannot be inherited
    fn method() { }
}

class Base {
    seal fn locked() { }  // Cannot be overridden
    fn normal() { }
}
```

**Implementation:** Compiler prevents inheritance/overriding during compilation. No VM changes.

---

## 7. Method Chaining

```droplet
class Builder {
    value: int
    
    fn add(n: int) -> self {
        self.value = self.value + n
        ret self
    }
    
    fn multiply(n: int) -> self {
        self.value = self.value * n
        ret self
    }
}

// Usage:
let result = Builder()
    .add(5)
    .multiply(2)
    .add(3)
```

**Implementation:** `self` type is syntactic sugar - compiler replaces with actual class type.

---

## 8. Type Checking and Casting

### 8.1 Type Checking
```droplet
let obj: obj = Dog("Rex", "Labrador")

if obj is Dog {
    println("It's a dog!")
}
```

**Implementation:**
- Store class name in `ObjInstance.className`
- `is` operator → Compare runtime class name
- Bytecode: `OP_GET_FIELD` (className), `OP_STRING_EQ`, compare

### 8.2 Type Casting
```droplet
let animal: Animal = Dog("Rex", "Labrador")
let dog = animal as Dog

if dog != null {
    println(dog.breed)
}
```

**Implementation:**
- Compiler inserts runtime type check
- On success: no-op (same object reference)
- On failure: return `null` or panic (based on variant)

---

## 9. Packages and Imports

```droplet
// File: com/example/math/Vector.drop
mod com.example.math

pub class Vector {
    pub x: float
    pub y: float
}

// File: main.drop
from com.example.math import Vector
from std.collections import List, Dict
from std.io import *

fn main() {
    let v = Vector()
}
```

**Implementation:**
1. Compiler resolves file paths from module names
2. Imports add symbols to namespace
3. All code compiled into single DBC file with mangled names
4. `mod` statement affects symbol prefixes

---

## 10. Foreign Function Interface (FFI)

### 10.1 FFI Declaration
```droplet
// Declare external library
@ffi("libmath.so")
class Math {
    static fn sqrt(x: float) -> float
    static fn pow(base: float, exp: float) -> float
}

// Declare C function
@ffi("libc.so.6", "strlen")
fn c_strlen(s: str) -> int
```

### 10.2 Current VM Support
Your VM already implements `OP_CALL_FFI` with these capabilities:

```cpp
// Supported signatures (sig parameter):
// 0 = int32(int32, int32)
// 1 = int32(int32)  
// 2 = double(double, double)
```

### 10.3 Extended FFI Design

```droplet
// Signature encoding extension needed:
@ffi("mylib.so", sig="iiff->f")  // int32, int32, float, float -> float
fn complex_calc(a: int, b: int, x: float, y: float) -> float

// String passing
@ffi("libc.so.6", sig="s->i")
fn strlen(s: str) -> int

// Void return
@ffi("mylib.so", sig="ii->v")
fn notify(code: int, value: int)
```

**Signature Format:**
- `i` = int32
- `f` = float/double
- `s` = string (const char*)
- `v` = void
- `->` separates args from return type

### 10.4 FFI Bytecode Generation

```droplet
let result = Math.sqrt(16.0)

// Compiles to:
OP_PUSH_CONST <libIndex>     // "libmath.so"
OP_PUSH_CONST <symIndex>     // "sqrt"
OP_PUSH_CONST <16.0>         // argument
OP_CALL_FFI <libIdx> <symIdx> <1> <3>  // argc=1, sig=3 (new: double->double)
```

### 10.5 FFI Memory Safety

```droplet
// Compiler generates automatic conversions:

// Droplet string -> C string
let s: str = "hello"
c_function(s)  
// Compiler: Extract ObjString.value.c_str(), pass pointer

// C return string -> Droplet string  
let result = c_get_string()
// Compiler: Wrap returned char* in new ObjString
```

### 10.6 Extended VM Support Needed

To fully support FFI design, VM needs these signature types:

```cpp
// In VM::run(), OP_CALL_FFI handler:
case 3: // double(double) - single double arg
case 4: // void(int32, int32) - void return
case 5: // const char*(const char*) - string I/O
case 6: // void*(...) - generic pointer
```

### 10.7 FFI Example Usage

```droplet
// Graphics library binding
@ffi("libSDL2.so")
class SDL {
    @ffi(sig="iiii->i")
    static fn CreateWindow(x: int, y: int, w: int, h: int) -> int
    
    @ffi(sig="i->v")
    static fn DestroyWindow(window: int)
}

// System calls
@ffi("libc.so.6")
class System {
    @ffi(sig="s->i")
    static fn system(command: str) -> int
    
    @ffi(sig="->i") 
    static fn getpid() -> int
}

fn main() {
    let pid = System.getpid()
    println("Process ID: " + pid)
    
    SDL.CreateWindow(0, 0, 800, 600)
}
```

---

## 11. Built-in Methods

### 11.1 List Methods
```droplet
class list[T] {
    fn len() -> int
    fn push(item: T)
    fn pop() -> T
    fn get(index: int) -> T
    fn set(index: int, value: T)
}
```

**Implementation:** Native functions registered in VM:
```cpp
vm.register_native("list$len", [](VM& vm, uint8_t argc) {
    Value listVal = vm.peek_back(0);
    ObjArray* arr = dynamic_cast<ObjArray*>(listVal.current_value.object);
    vm.pop_back();
    vm.push_back(Value::createINT(arr->value.size()));
});
```

### 11.2 Dictionary Methods
```droplet
class dict[K,V] {
    fn has(key: K) -> bool
    fn del(key: K)
    fn keys() -> list[K]
    fn values() -> list[V]
}
```

### 11.3 String Methods
```droplet
class str {
    fn len() -> int           // OP_STRING_LENGTH
    fn substr(start: int, len: int) -> str  // OP_STRING_SUBSTR
    fn charAt(index: int) -> str  // OP_STRING_GET_CHAR
    fn concat(other: str) -> str  // OP_STRING_CONCAT
}
```

---

## 12. Operator Overloading

### 12.1 Supported Operators
```droplet
class Vector {
    x: float
    y: float
    
    op +(other: Vector) -> Vector {
        ret Vector(self.x + other.x, self.y + other.y)
    }
    
    op -(other: Vector) -> Vector {
        ret Vector(self.x - other.x, self.y - other.y)
    }
    
    op ==(other: Vector) -> bool {
        ret self.x == other.x && self.y == other.y
    }
    
    op [](index: int) -> float {
        if index == 0 { ret self.x }
        if index == 1 { ret self.y }
        ret 0.0
    }
}
```

**Implementation:**
- Compiler translates `a + b` → `a.op$add(b)`
- Operator methods are regular methods with special names
- Built-in operators (`OP_ADD`, etc.) handle primitives

**Operator Method Names:**
- `op$add`, `op$sub`, `op$mul`, `op$div`, `op$mod`
- `op$eq`, `op$neq`, `op$lt`, `op$gt`, `op$lte`, `op$gte`
- `op$neg` (unary `-`), `op$not`
- `op$index_get`, `op$index_set`

---

## 13. Control Flow

All control flow compiles to existing opcodes:

```droplet
// if-else
if condition {
    action1()
} else {
    action2()
}

// Compiles to:
OP_LOAD_LOCAL <condition>
OP_JUMP_IF_FALSE <else_label>
  <action1 code>
  OP_JUMP <end_label>
else_label:
  <action2 code>
end_label:

// for loop
for i in 0..10 {
    println(i)
}

// Compiles to:
OP_PUSH_CONST <0>
OP_STORE_LOCAL <i>
loop_start:
  OP_LOAD_LOCAL <i>
  OP_PUSH_CONST <10>
  OP_LT
  OP_JUMP_IF_FALSE <loop_end>
    <body code>
    OP_LOAD_LOCAL <i>
    OP_PUSH_CONST <1>
    OP_ADD
    OP_STORE_LOCAL <i>
  OP_JUMP <loop_start>
loop_end:

// while loop
while condition {
    action()
}

// Compiles to:
loop_start:
  OP_LOAD_LOCAL <condition>
  OP_JUMP_IF_FALSE <loop_end>
    <action code>
  OP_JUMP <loop_start>
loop_end:
```

---

## 14. Example Programs

### 14.1 Simple Class
```droplet
class Rectangle {
    width: float
    height: float
    
    new(w: float, h: float) {
        self.width = w
        self.height = h
    }
    
    fn area() -> float {
        ret self.width * self.height
    }
    
    fn perimeter() -> float {
        ret 2.0 * (self.width + self.height)
    }
}

fn main() {
    let rect = Rectangle(5.0, 3.0)
    println("Area: " + rect.area())
    println("Perimeter: " + rect.perimeter())
}
```

### 14.2 Inheritance Example
```droplet
class Shape {
    color: str
    
    new(c: str) {
        self.color = c
    }
    
    fn describe() -> str {
        ret "A " + self.color + " shape"
    }
}

class Circle : Shape {
    radius: float
    
    new(c: str, r: float) {
        Shape.new(c)
        self.radius = r
    }
    
    fn area() -> float {
        ret 3.14159 * self.radius * self.radius
    }
    
    fn describe() -> str {
        ret Shape.describe() + " (circle)"
    }
}

fn main() {
    let circle = Circle("red", 5.0)
    println(circle.describe())
    println("Area: " + circle.area())
}
```

### 14.3 Generics Example
```droplet
class Stack[T] {
    items: list[T]
    
    new() {
        self.items = []
    }
    
    fn push(item: T) {
        self.items.push(item)
    }
    
    fn pop() -> T {
        ret self.items.pop()
    }
    
    fn isEmpty() -> bool {
        ret self.items.len() == 0
    }
}

fn main() {
    let intStack = Stack[int]()
    intStack.push(10)
    intStack.push(20)
    println(intStack.pop())  // 20
    
    let strStack = Stack[str]()
    strStack.push("hello")
    strStack.push("world")
    println(strStack.pop())  // "world"
}
```

### 14.4 FFI Example
```droplet
@ffi("libm.so.6")
class Math {
    @ffi(sig="f->f")
    static fn sqrt(x: float) -> float
    
    @ffi(sig="ff->f")
    static fn pow(base: float, exp: float) -> float
}

fn main() {
    let x = 16.0
    let root = Math.sqrt(x)
    println("sqrt(" + x + ") = " + root)
    
    let result = Math.pow(2.0, 8.0)
    println("2^8 = " + result)
}
```

### 14.5 Composition Pattern (Alternative to Multiple Inheritance)
```droplet
// Instead of mixins, use composition
class FlyableBehavior {
    fn fly() { 
        println("Flying!") 
    }
}

class SwimmableBehavior {
    fn swim() { 
        println("Swimming!") 
    }
}

class Duck {
    name: str
    flyable: FlyableBehavior
    swimmable: SwimmableBehavior
    
    new(name: str) {
        self.name = name
        self.flyable = FlyableBehavior()
        self.swimmable = SwimmableBehavior()
    }
    
    fn fly() {
        self.flyable.fly()
    }
    
    fn swim() {
        self.swimmable.swim()
    }
}

fn main() {
    let duck = Duck("Donald")
    duck.fly()
    duck.swim()
}
```

---

## 15. Compilation Strategy

### 15.1 Class Compilation Pipeline

1. **Parse:** AST with class definitions
2. **Name Mangling:**
    - Methods: `ClassName$$methodName`
    - Static fields: `ClassName$$fieldName`
3. **Field Layout:** Compute offset for each field (parent fields first)
4. **Method Table:** Build dispatch table mapping method names to function indices
5. **Code Generation:**
    - Constructor → Function that calls `OP_NEW_OBJECT` + `OP_SET_FIELD`
    - Methods → Regular functions with `self` as first parameter
6. **DBC Output:** Functions with mangled names, global constants for class metadata

### 15.2 Inheritance Compilation

```droplet
class Animal {
    name: str  // offset 0
}

class Dog : Animal {
    breed: str  // offset 1
}

// Compiled field access:
dog.name  → OP_GET_FIELD <nameIdx=0>
dog.breed → OP_GET_FIELD <breedIdx=1>
```

### 15.3 Generic Monomorphization

```droplet
class Box[T] {
    value: T
    fn get() -> T { ret self.value }
}

let intBox = Box[int](42)
let strBox = Box[str]("hi")

// Compiler generates:
// Box_int class with:
//   - value: int
//   - fn Box_int$$get() -> int
// Box_str class with:
//   - value: str  
//   - fn Box_str$$get() -> str
```

---

## 16. Keywords

```
as bool break case class const continue else seal
loop float fn if import with is int let match mod
new null op priv prot pub ret self static str use while
get set parent
```

---

## 17. Operator Precedence (high to low)

1. `()` `[]` `.`
2. `!` `-` (unary)
3. `*` `/` `%`
4. `+` `-`
5. `<` `<=` `>` `>=`
6. `==` `!=`
7. `&&`
8. `||`
9. `=` `+=` `-=` etc.

---
