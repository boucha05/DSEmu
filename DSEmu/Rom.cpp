#include "Rom.h"
#include <cstdio>
#include <vector>

namespace
{
    bool readFile(std::vector<uint8_t>& buffer, const char* path, size_t maxSize = SIZE_MAX)
    {
        FILE* file = fopen(path, "rb");
        EMU_VERIFY(file);

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);
        size = std::min(size, maxSize);

        buffer.clear();
        if (size)
        {
            buffer.resize(size, 0);
            size = fread(&buffer[0], 1, size, file);
            buffer.resize(size);
        }
        fclose(file);

        return size > 0;
    }
}

namespace nds
{
    bool Rom::create(const char* path)
    {
        EMU_VERIFY(readFile(mContent, path));
        EMU_VERIFY(getHeader(mHeader, mContent.data(), mContent.size()));
        EMU_VERIFY(getDescription(mDescription, mHeader));

        // Swap 32-bit words into native endian to make 32-bit memory operations faster
        EMU_VERIFY((mContent.size() & 3) == 0);
        NDS_SWAP_ARRAY(reinterpret_cast<uint32_t*>(mContent.data()), emu::divideUp(mContent.size(), static_cast<size_t>(4)));

        return true;
    }

    bool Rom::getHeader(Header& header, const void* data, size_t size)
    {
        EMU_VERIFY(size >= sizeof(header));
        memcpy(&header, data, sizeof(Header));

        NDS_SWAP_INLINE(header.ARM9ROMOffset);
        NDS_SWAP_INLINE(header.ARM9EntryAddress);
        NDS_SWAP_INLINE(header.ARM9RAMAddress);
        NDS_SWAP_INLINE(header.ARM9Size);
        NDS_SWAP_INLINE(header.ARM7ROMOffset);
        NDS_SWAP_INLINE(header.ARM7EntryAddress);
        NDS_SWAP_INLINE(header.ARM7RAMAddress);
        NDS_SWAP_INLINE(header.ARM7Size);
        NDS_SWAP_INLINE(header.FNTOffset);
        NDS_SWAP_INLINE(header.FNTSize);
        NDS_SWAP_INLINE(header.FATOffset);
        NDS_SWAP_INLINE(header.FATSize);
        NDS_SWAP_INLINE(header.FileARM9OverlayOffset);
        NDS_SWAP_INLINE(header.FileARM9OverlaySize);
        NDS_SWAP_INLINE(header.FileARM7OverlayOffset);
        NDS_SWAP_INLINE(header.FileARM7OverlaySize);
        NDS_SWAP_INLINE(header.Port40001A4NormalCommands);
        NDS_SWAP_INLINE(header.Port40001A4KEY1Commands);
        NDS_SWAP_INLINE(header.IconTitleOffset);
        NDS_SWAP_INLINE(header.SecureAreaChecksum);
        NDS_SWAP_INLINE(header.SecureAreaDelay);
        NDS_SWAP_INLINE(header.ARM9AutoLoadListRAMAddress);
        NDS_SWAP_INLINE(header.ARM7AutoLoadListRAMAddress);
        NDS_SWAP_INLINE(header.TotalUsedROMSize);
        NDS_SWAP_INLINE(header.ROMHeaderSize);
        NDS_SWAP_INLINE(header.NintendoLogoChecksum);
        NDS_SWAP_INLINE(header.HeaderChecksum);
        NDS_SWAP_INLINE(header.DebugROMOffset);
        NDS_SWAP_INLINE(header.DebugSize);
        NDS_SWAP_INLINE(header.DebugRAMAddress);
        return true;
    }

    bool Rom::getDescription(Description& description, const Header& header)
    {
        strncpy(description.title, header.GameTitle, sizeof(header.GameTitle));
        return true;
    }

    bool Rom::readHeader(Header& header, const char* path)
    {
        std::vector<uint8_t> buffer;
        EMU_VERIFY(readFile(buffer, path, sizeof(Header)));
        return getHeader(header, buffer.data(), buffer.size());
    }

    bool Rom::readDescription(Description& description, const char* path)
    {
        Header header;
        EMU_VERIFY(readHeader(header, path));
        return getDescription(description, header);
    }
}