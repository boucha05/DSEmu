#pragma once

#include "ConfigWindows.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if defined(DEBUG) || defined(_DEBUG)
#define EMU_DEBUG       1
#define EMU_ASSERT(e)   emu::assertValid(!!(e), #e)
#else
#define EMU_DEBUG       0
#define EMU_ASSERT(e)
#endif
#define EMU_VERIFY(e)   if (e) ; else return false

#define EMU_INVOKE_ONCE(e)                  \
{                                           \
    static bool __invoked_before = false;   \
    if (!__invoked_before)                  \
    {                                       \
        __invoked_before = true;            \
        e;                                  \
    }                                       \
}

#if EMU_DEBUG
#define EMU_NOT_IMPLEMENTED()                       EMU_INVOKE_ONCE(emu::notImplemented(__FUNCTION__))
#else
#define EMU_NOT_IMPLEMENTED()
#endif
#define EMU_NOT_IMPLEMENTED_COND(cond)              if (!(cond)) ; else { EMU_NOT_IMPLEMENTED() }

#define EMU_BITS_BLEND(val1, val2, mask)            (((val1) & ~(mask)) | ((val2) & (mask)))
#define EMU_BITS_MASK(shift, size)                  (((1 << (size)) - 1) << (shift))
#define EMU_BITS_GET(shift, size, val)              (((val) >> (shift)) & ((1 << (size)) - 1))
#define EMU_BITS_SET(shift, size, val, field)       (EMU_BITS_BLEND((val), (field) << (shift), EMU_BITS_MASK(shift, size)))
#define EMU_BITS_MASK_FIELD(name)                   (EMU_BITS_MASK(name##_SHIFT, name##_SIZE))
#define EMU_BITS_GET_FIELD(name, val)               (EMU_BITS_GET(name##_SHIFT, name##_SIZE, val))
#define EMU_BITS_SET_FIELD(name, val, field)        (EMU_BITS_SET(name##_SHIFT, name##_SIZE, val, field))
#define EMU_BIT(shift)                              (1U << (shift))
#define EMU_BIT_GET(shift, val)                     (EMU_BITS_GET(shift, 1, val))
#define EMU_BIT_SET(shift, val, field)              (EMU_BITS_SET(shift, 1, val, field))
#define EMU_BIT_FIELD(name)                         (EMU_BIT(name##_SHIFT, 1))
#define EMU_BIT_GET_FIELD(name, val)                (EMU_BIT_GET(name##_SHIFT, 1, val))
#define EMU_BIT_SET_FIELD(name, val, field)         (EMU_BIT_SET(name##_SHIFT, 1, val, field))

#if EMU_CONFIG_LITTLE_ENDIAN
#define EMU_SWAP_LITTLE_ENDIAN(expr)                (expr)
#define EMU_SWAP_LITTLE_ENDIAN_INLINE(expr)         (expr)
#define EMU_SWAP_LITTLE_ENDIAN_ARRAY(expr,count)    (expr)
#define EMU_SWAP_BIG_ENDIAN(expr)                   (emu::swap(expr))
#define EMU_SWAP_BIG_ENDIAN_INLINE(expr)            (emu::swapInline(expr))
#define EMU_SWAP_BIG_ENDIAN_ARRAY(expr,count)       (emu::swap((expr),(count)))
#else
#define EMU_SWAP_LITTLE_ENDIAN(expr)                (emu::swap(expr))
#define EMU_SWAP_LITTLE_ENDIAN_INLINE(expr)         (emu::swapInline(expr))
#define EMU_SWAP_LITTLE_ENDIAN_ARRAY(expr,count)    (emu::swap((expr),(count)))
#define EMU_SWAP_BIG_ENDIAN(expr)                   (expr)
#define EMU_SWAP_BIG_ENDIAN_INLINE(expr)            (expr)
#define EMU_SWAP_BIG_ENDIAN_ARRAY(expr,count)       (expr)
#endif

#define EMU_DIR_SEPARATOR       EMU_CONFIG_DIR_SEPARATOR

#define EMU_UNUSED(expr)        (void)expr

#define EMU_GET_MACRO_ARG_TYPE(arg)     emu::RemoveTypeParenthesis<void arg>::Type

namespace emu
{
    template<typename>
    struct RemoveTypeParenthesis;

    template<typename T>
    struct RemoveTypeParenthesis<void(T)>
    {
        typedef T Type;
    };

    template <typename T>
    void swapInline(T& val)
    {
        val = swap(val);
    }

    inline int16_t swap(int16_t val)
    {
        return EMU_PLATFORM_SWAP16(val);
    }

    inline uint16_t swap(uint16_t val)
    {
        return EMU_PLATFORM_SWAP16(val);
    }

    inline int32_t swap(int32_t val)
    {
        return EMU_PLATFORM_SWAP32(val);
    }

    inline uint32_t swap(uint32_t val)
    {
        return EMU_PLATFORM_SWAP32(val);
    }

    inline int64_t swap(int64_t val)
    {
        return EMU_PLATFORM_SWAP64(val);
    }

    inline uint64_t swap(uint64_t val)
    {
        return EMU_PLATFORM_SWAP64(val);
    }

    template <typename T>
    void swap(T* val, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
            swapInline(val[index]);
    }

    template <typename T>
    T alignUp(T value, uint32_t alignment)
    {
        T mask = static_cast<T>(alignment) - 1;
        auto result = (value + mask) & ~mask;
        return result;
    }

    template <typename T>
    T divideUp(T value, uint32_t alignment)
    {
        T mask = static_cast<T>(alignment) - 1;
        auto result = (value + mask) / alignment;
        return result;
    }

    void assertValid(bool valid, const char* msg);
    void notImplemented(const char* function);

    class IContext;
    class ISystem;

    class ISystem
    {
    public:
        virtual IContext* loadGame(const char* path) = 0;
        virtual void unloadGame(IContext& instance) = 0;
    };

    class IContext
    {
    public:
        virtual void reset() = 0;
        virtual void executeFrame() = 0;
    };
}