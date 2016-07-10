#pragma once

#include "NDS.h"
#include <vector>

namespace nds
{
    class Rom
    {
    public:
        struct Description
        {
            char        title[13];

        };

        struct Header
        {
            char        GameTitle[12];
            char        GameCode[4];
            char        Makercode[2];
            uint8_t     Unitcode;
            uint8_t     EncryptionSeedSelect;
            uint8_t     DeviceCapacity;
            uint8_t     Reserved015[7];
            uint8_t     Reserved01C;
            uint8_t     NDSRegion;
            uint8_t     ROMVersion;
            uint8_t     Autostart;
            uint32_t    ARM9ROMOffset;
            uint32_t    ARM9EntryAddress;
            uint32_t    ARM9RAMAddress;
            uint32_t    ARM9Size;
            uint32_t    ARM7ROMOffset;
            uint32_t    ARM7EntryAddress;
            uint32_t    ARM7RAMAddress;
            uint32_t    ARM7Size;
            uint32_t    FNTOffset;
            uint32_t    FNTSize;
            uint32_t    FATOffset;
            uint32_t    FATSize;
            uint32_t    FileARM9OverlayOffset;
            uint32_t    FileARM9OverlaySize;
            uint32_t    FileARM7OverlayOffset;
            uint32_t    FileARM7OverlaySize;
            uint32_t    Port40001A4NormalCommands;
            uint32_t    Port40001A4KEY1Commands;
            uint32_t    IconTitleOffset;
            uint16_t    SecureAreaChecksum;
            uint16_t    SecureAreaDelay;
            uint32_t    ARM9AutoLoadListRAMAddress;
            uint32_t    ARM7AutoLoadListRAMAddress;
            uint8_t     SecureAreaDisable[8];
            uint32_t    TotalUsedROMSize;
            uint32_t    ROMHeaderSize;
            uint8_t     Reserved088[0x38];
            uint8_t     NintendoLogo[0x9c];
            uint16_t    NintendoLogoChecksum;
            uint16_t    HeaderChecksum;
            uint32_t    DebugROMOffset;
            uint32_t    DebugSize;
            uint32_t    DebugRAMAddress;
            uint8_t     Reserved16C[4];
            uint8_t     Reserved170[0x90];
        };

        bool create(const char* path);
        static bool getHeader(Header& header, const void* data, size_t size);
        static bool getDescription(Description& description, const Header& header);
        static bool readHeader(Header& header, const char* path);
        static bool readDescription(Description& description, const char* path);

        const std::vector<char>& getContent() const
        {
            return mContent;
        }

        const Header& getHeader() const
        {
            return mHeader;
        }

        const Description& getDescription() const
        {
            return mDescription;
        }

    private:
        std::vector<char>   mContent;
        Header              mHeader;
        Description         mDescription;
    };
}
