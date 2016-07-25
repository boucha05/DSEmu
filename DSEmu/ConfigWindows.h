#pragma once

#define EMU_CONFIG_LITTLE_ENDIAN    1
#define EMU_CONFIG_BIG_ENDIAN       0
#define EMU_CONFIG_DIR_SEPARATOR    '\\'

#define _CRT_SECURE_NO_WARNINGS

#include <intrin.h>
#include <stdint.h>
#include <stdlib.h>

#define EMU_PLATFORM_SWAP16(val)    (_byteswap_ushort(val))
#define EMU_PLATFORM_SWAP32(val)    (_byteswap_ulong(val))
#define EMU_PLATFORM_SWAP64(val)    (_byteswap_uint64(val))

namespace emu
{
    namespace platform
    {
        inline bool findFirstBit(size_t& pos, uint32_t mask)
        {
            unsigned long lpos;
            bool result = _BitScanForward(&lpos, mask) != 0;
            pos = static_cast<size_t>(lpos);
            return result;
        }

#if defined(_WIN64)
        inline bool findFirstBit(size_t& pos, uint64_t mask)
        {
            unsigned long lpos;
            bool result = _BitScanForward64(&lpos, mask) != 0;
            pos = static_cast<size_t>(lpos);
            return result;
        }
#else
        inline bool findFirstBit(size_t& pos, uint64_t mask)
        {
            if (findFirstBit(pos, static_cast<uint32_t>(mask)))
            {
                return true;
            }
            else if (findFirstBit(pos, static_cast<uint32_t>(mask >> 32ULL)))
            {
                pos += 32;
                return true;
            }
            return false;
        }
#endif

        inline bool findLastBit(size_t& pos, uint32_t mask)
        {
            unsigned long lpos;
            bool result = _BitScanReverse(&lpos, mask) != 0;
            pos = static_cast<size_t>(lpos);
            return result;
        }

#if defined(_WIN64)
        inline bool findLastBit(size_t& pos, uint64_t mask)
        {
            unsigned long lpos;
            bool result = _BitScanReverse64(&lpos, mask) != 0;
            pos = static_cast<size_t>(lpos);
            return result;
        }
#else
        inline bool findLastBit(size_t& pos, uint64_t mask)
        {
            if (findLastBit(pos, static_cast<uint32_t>(mask >> 32ULL)))
            {
                pos += 32;
                return true;
            }
            else if (findLastBit(pos, static_cast<uint32_t>(mask)))
            {
                return true;
            }
            return false;
        }
#endif
    }
}
