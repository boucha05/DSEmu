#pragma once

#include "Core.h"
#include "Delegate.h"
#include <vector>

namespace emu
{
    class MemoryBus
    {
    public:
        typedef Delegate<uint8_t(uint32_t)>         Read8Delegate;
        typedef Delegate<uint16_t(uint32_t)>        Read16Delegate;
        typedef Delegate<uint32_t(uint32_t)>        Read32Delegate;
        typedef Delegate<void(uint32_t, uint8_t)>   Write8Delegate;
        typedef Delegate<void(uint32_t, uint16_t)>  Write16Delegate;
        typedef Delegate<void(uint32_t, uint32_t)>  Write32Delegate;

        struct Accessor
        {
            std::string         mName;
            uint8_t*            mMemory;
        };

        struct ReadAccessor : public Accessor
        {
            ReadAccessor& setName(const char* value)
            {
                mName = value;
                return *this;
            }

            ReadAccessor& setMemory(const void* value)
            {
                mMemory = static_cast<uint8_t*>(const_cast<void*>(value));
                return *this;
            }

            ReadAccessor& setRead8(const Read8Delegate& value)
            {
                mMemory = nullptr;
                mRead8 = value;
                return *this;
            }

            ReadAccessor& setRead16(const Read16Delegate& value)
            {
                mMemory = nullptr;
                mRead16 = value;
                return *this;
            }

            ReadAccessor& setRead32(const Read32Delegate& value)
            {
                mMemory = nullptr;
                mRead32 = value;
                return *this;
            }

            Read8Delegate       mRead8;
            Read16Delegate      mRead16;
            Read32Delegate      mRead32;
        };

        struct WriteAccessor : public Accessor
        {
            WriteAccessor& setName(const char* value)
            {
                mName = value;
                return *this;
            }

            WriteAccessor& setMemory(void* value)
            {
                mMemory = static_cast<uint8_t*>(value);
                return *this;
            }

            WriteAccessor& setWrite8(const Write8Delegate& value)
            {
                mMemory = nullptr;
                mWrite8 = value;
                return *this;
            }

            WriteAccessor& setWrite16(const Write16Delegate& value)
            {
                mMemory = nullptr;
                mWrite16 = value;
                return *this;
            }

            WriteAccessor& setWrite32(const Write32Delegate& value)
            {
                mMemory = nullptr;
                mWrite32 = value;
                return *this;
            }

            Write8Delegate      mWrite8;
            Write16Delegate     mWrite16;
            Write32Delegate     mWrite32;
        };

        struct ReadWriteAccessor
        {
            ReadWriteAccessor& setName(const char* value)
            {
                read.setName(value);
                write.setName(value);
                return *this;
            }

            ReadWriteAccessor& setMemory(void* value)
            {
                read.setMemory(value);
                write.setMemory(value);
                return *this;
            }

            ReadAccessor        read;
            WriteAccessor       write;
        };

        struct Page
        {
            struct Item
            {
                uint32_t        mBase;
                uint32_t        mLimit;
                uint32_t        mOffset;
                Accessor*       mAccessor;
            };

            std::vector<Item>   mItems;
        };

        MemoryBus();
        ~MemoryBus();
        bool create(uint32_t memSizeLog2, uint32_t pageSizeLog2);
        bool addRange(uint32_t base, uint32_t size, ReadAccessor& readAccessor);
        bool addRange(uint32_t base, uint32_t size, WriteAccessor& writeAccessor);
        bool addRange(uint32_t base, uint32_t size, ReadAccessor& readAccessor, WriteAccessor& writeAccessor);
        bool addRange(uint32_t base, uint32_t size, ReadWriteAccessor& readWriteAccessor);
        uint8_t read8(uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint32_t read32(uint32_t addr);
        void write8(uint32_t addr, uint8_t value);
        void write16(uint32_t addr, uint16_t value);
        void write32(uint32_t addr, uint32_t value);
        void export8(uint32_t addr, void* data, size_t count);
        void export16(uint32_t addr, void* data, size_t count);
        void export32(uint32_t addr, void* data, size_t count);
        void import8(uint32_t addr, const void* data, size_t count);
        void import16(uint32_t addr, const void* data, size_t count);
        void import32(uint32_t addr, const void* data, size_t count);

    private:
        bool addRange(std::vector<Page>& pageTable, uint32_t base, uint32_t size, Accessor& accessor);
        Page::Item* findPageItem(std::vector<Page>& pageTable, uint32_t addr);
        uint8_t invalidRead8(uint32_t addr);
        uint16_t invalidRead16(uint32_t addr);
        uint32_t invalidRead32(uint32_t addr);
        void invalidWrite8(uint32_t addr, uint8_t value);
        void invalidWrite16(uint32_t addr, uint16_t value);
        void invalidWrite32(uint32_t addr, uint32_t value);

        uint32_t            mMemSizeLog2;
        uint32_t            mPageSizeLog2;
        uint32_t            mMemLimit;
        uint32_t            mPageLimit;
        ReadWriteAccessor   mInvalidMemory;
        std::vector<Page>   mReadPages;
        std::vector<Page>   mWritePages;
    };
}