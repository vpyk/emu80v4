// Minimal Z definitions for z80 library
// Compiled by Viktor Pykhonin

#ifndef ZDEFS_H
#define ZDEFS_H

#include <cstdint>
#include <cstddef>

typedef bool zbool;
typedef char zchar;
typedef int  zsint;
typedef int8_t zsint8;
typedef unsigned zuint;
typedef uint8_t zuint8;
typedef uint16_t zuint16;
typedef uint32_t zuint32;
typedef size_t zusize;
typedef unsigned* zuintptr;

#define Z_NULL nullptr

#define Z_ALWAYS_INLINE inline
#define Z_CAST(type) type

#define Z_z_EMPTY(dummy)
#define Z_EMPTY Z_z_EMPTY(~)

#define Z_EXTERN_C_BEGIN extern "C" {
#define Z_EXTERN_C_END   }

#define Z_MEMBER_OFFSET(type, member) zusize(zuintptr(&(static_cast<type *>(Z_NULL))->member))

#define Z_UINT8_ROTATE_LEFT(value, rotation) (zuint8(((value) << (rotation)) | ((value) >> (8 - (rotation)))))
#define Z_UINT8_ROTATE_RIGHT(value, rotation) (zuint8(((value) >> (rotation)) | ((value) << (8 - (rotation)))))

#define Z_z_SUFFIX_U(lht) lht##U
#define Z_UINT16(lht) Z_z_SUFFIX_U (lht)
#define Z_UINT32(lht) Z_z_SUFFIX_U (lht)

#define Z_UINT16_BIG_ENDIAN(value) (zuint16(((value) << 8) | ((value) >> 8)))
#define Z_UINT32_BIG_ENDIAN(value) (zuint32((((value) << 24)) | (((value) >> 24)) | (((value) << 8) & (zuint32(255) << 16)) | (((value) >> 8) & (zuint32(255) << 8))))

#define Z_UNUSED(variable) (void)variable;

#define Z_USIZE
#define Z_USIZE_MAXIMUM SIZE_MAX

typedef union {
    zuint16 uint16_value;
    zuint16 uint16_array[1];

    struct {zuint16 at_0;
    } uint16_values;

    zuint8 uint8_array[2];

    struct {
            zuint8 at_0;
            zuint8 at_1;
    } uint8_values;
} ZInt16;


typedef union {
    zuint32 uint32_value;
    zuint32 uint32_array[1];

    struct {zuint32 at_0;
    } uint32_values;

    zuint16 uint16_array[2];

    struct {
            zuint16 at_0;
            zuint16 at_1;
    } uint16_values;

    zuint8 uint8_array[4];

    struct {
            zuint8 at_0;
            zuint8 at_1;
            zuint8 at_2;
            zuint8 at_3;
    } uint8_values;
} ZInt32;

#endif // ZDEFS_H
