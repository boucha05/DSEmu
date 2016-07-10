#pragma once

#define EMU_CONFIG_LITTLE_ENDIAN    1
#define EMU_CONFIG_DIR_SEPARATOR    '\\'

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdlib.h>

#define EMU_PLATFORM_SWAP16(val)    (_byteswap_ushort(val))
#define EMU_PLATFORM_SWAP32(val)    (_byteswap_ulong(val))
#define EMU_PLATFORM_SWAP64(val)    (_byteswap_uint64(val))
