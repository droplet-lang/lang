#include <stdint.h>

#ifdef _WIN32
#define API __declspec(dllexport)
#define CALL_CONV __cdecl
#else
#define API
#define CALL_CONV
#endif

API int64_t CALL_CONV add_numbers(int64_t a, int64_t b) {
    return a + b;
}
