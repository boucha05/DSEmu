#pragma once

#include "ConfigWindows.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if defined(DEBUG) || defined(_DEBUG)
#include <assert.h>
#define EMU_ASSERT(e)   emu::Assert(!!(e), #e)
#else
#define EMU_ASSERT(e)
#endif
#define EMU_VERIFY(e)   if (e) ; else return false

#define EMU_BIT(n)      (1 << (n))

#if EMU_CONFIG_LITTLE_ENDIAN
#define EMU_SWAP_LITTLE_ENDIAN(expr)            (expr)
#define EMU_SWAP_LITTLE_ENDIAN_INLINE(expr)     (expr)
#define EMU_SWAP_BIG_ENDIAN(expr)               (emu::swap(expr))
#define EMU_SWAP_BIG_ENDIAN_INLINE(expr)        (emu::swapInline(expr))
#else
#define EMU_SWAP_LITTLE_ENDIAN(expr)            (emu::swap(expr))
#define EMU_SWAP_LITTLE_ENDIAN_INLINE(expr)     (emu::swapInline(expr))
#define EMU_SWAP_BIG_ENDIAN(expr)               (expr)
#define EMU_SWAP_BIG_ENDIAN_INLINE(expr)        (expr)
#endif

#define EMU_DIR_SEPARATOR   EMU_CONFIG_DIR_SEPARATOR

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