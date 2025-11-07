# DRAFT 1
1. VM-based (stack based)
2. Pure OOP
3. GC-driven
4. Supports only Single Inheritance (let's not deal with Diamond problem, we can further add concept of mixins)
5. Supports Dynamic Dispatches
6. Some inbulit objects: Object, String, Array, etc.
7. Native Function Support (see more in below sections on impl details)

## Native Function Support
This will let our language be very much extensible allowing us to delegate most of 
the platform specific feature to Native Code. We will need to also add feature of FFI
in order to make the user also use this native function features.

Example:
```cpp
void nativePrint(VM* vm, int argCount) {
    for(int i = 0; i < argCount; ++i) {
        Value val = vm->pop();
        switch(val.type) {
            case ValueType::INT:    std::cout << val.i; break;
            case ValueType::DOUBLE: std::cout << val.d; break;
            case ValueType::STRING: std::cout << val.str->c_str(); break;
            default:                std::cout << "nil"; break;
        }
        if(i < argCount - 1) std::cout << " ";
    }
    std::cout << std::endl;
    vm->push(Value{ValueType::NIL}); // return NIL
}

// Register
registerNative(&vm, "print", nativePrint);
```

And we can, in our program:
```
print("Hello World");          // 1 argument
print("Sum:", 10 + 5);        // multiple arguments
```

### FFI
Let's say, user want to use the sqrt function of C.
```c
#include <math.h>

double my_sqrt(double x) {
    return sqrt(x);
}
```

They need to generate the shared library using the code in any of these language.

```
import "ffi"

var lib = ffi.load("math_extra.so")
var sqrt_fn = lib.func("my_sqrt", "double(double)")

var result = sqrt_fn(25.0)
print(result)  # 5.0
```

This will let us create extensions for sqlite, and other apis that have a C/C++/Rust api.