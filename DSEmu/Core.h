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

#define EMU_NOT_IMPLEMENTED()       EMU_INVOKE_ONCE(emu::notImplemented(__FUNCTION__))

#define EMU_BITS_MASK(bits)         (((1U << bits##_SIZE) - 1U) << (bits##_SHIFT))
#define EMU_BITS_READ(val, bits)    (((v) & EMU_BITS_MASK(bits)) >> bits##_SHIFT)
#define EMU_BITS_WRITE(val, bits)   (((v) << bits##_SHIFT) & EMU_BITS_MASK(bits))
#define EMU_BIT(shift)              (1U << (shift))

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

namespace emu
{
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