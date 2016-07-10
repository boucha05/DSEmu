#pragma once

#include "Core.h"

#define NDS_SWAP(val)               (EMU_SWAP_LITTLE_ENDIAN(val))
#define NDS_SWAP_INLINE(val)        (EMU_SWAP_LITTLE_ENDIAN_INLINE(val))
#define NDS_SWAP_ARRAY(val,count)   (EMU_SWAP_LITTLE_ENDIAN_ARRAY(val,count))

namespace nds
{
    class Rom;
    class System;

    static const uint32_t   DOTS_VISIBLE_H = 256;
    static const uint32_t   DOTS_VISIBLE_V = 192;
    static const uint32_t   DOTS_BLANKING_H = 99;
    static const uint32_t   DOTS_BLANKING_V = 71;
    static const uint32_t   DOTS_TOTAL_H = DOTS_VISIBLE_H + DOTS_BLANKING_H;
    static const uint32_t   DOTS_TOTAL_V = DOTS_VISIBLE_V + DOTS_BLANKING_V;
    static const uint32_t   DOTS_PER_FRAME = DOTS_TOTAL_H * DOTS_TOTAL_V;

    static const uint32_t   ARM7_TICKS_PER_DOT = 6;
    static const uint32_t   ARM7_TICKS_PER_FRAME = ARM7_TICKS_PER_DOT * DOTS_PER_FRAME;

    static const uint32_t   ARM9_TICKS_PER_DOT = 12;
    static const uint32_t   ARM9_TICKS_PER_FRAME = ARM9_TICKS_PER_DOT * DOTS_PER_FRAME;

    static const uint32_t   FRAMES_PER_SEC = 60;
    static const uint32_t   TICKS_PER_FRAME = ARM9_TICKS_PER_FRAME;
    static const uint32_t   TICKS_PER_SEC = TICKS_PER_FRAME * FRAMES_PER_SEC;

    static const uint32_t   MAIN_RAM_SIZE = 0x400000;

    emu::ISystem& getSystem();
}
