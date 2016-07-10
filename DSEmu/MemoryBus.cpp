#include "MemoryBus.h"

namespace
{
    using namespace emu;

    bool validatePage(MemoryBus::Page& page, uint32_t pageBase, uint32_t pageSize)
    {
        uint32_t addr = pageBase;
        for (auto item : page.mItems)
        {
            EMU_VERIFY(item.mBase == addr);
            EMU_VERIFY(item.mLimit > item.mBase);
            addr = item.mLimit + 1;
        }
        EMU_VERIFY(addr == pageBase + pageSize);
        return true;
    }
}

namespace emu
{
    MemoryBus::MemoryBus()
    {
    }

    MemoryBus::~MemoryBus()
    {
    }

    bool MemoryBus::create(uint32_t memSizeLog2, uint32_t pageSizeLog2)
    {
        EMU_VERIFY(memSizeLog2 <= 32);
        EMU_VERIFY(pageSizeLog2 <= memSizeLog2);

        mMemSizeLog2 = memSizeLog2;
        mMemLimit = (1 << memSizeLog2) - 1;
        mPageSizeLog2 = pageSizeLog2;
        mPageLimit = (1 << pageSizeLog2) - 1;

        mInvalidMemory.setName("Invalid");
        mInvalidMemory.read.setRead8(Read8Delegate::make<MemoryBus, &MemoryBus::invalidRead8>(this));
        mInvalidMemory.read.setRead16(Read16Delegate::make<MemoryBus, &MemoryBus::invalidRead16>(this));
        mInvalidMemory.read.setRead32(Read32Delegate::make<MemoryBus, &MemoryBus::invalidRead32>(this));
        mInvalidMemory.write.setWrite8(Write8Delegate::make<MemoryBus, &MemoryBus::invalidWrite8>(this));
        mInvalidMemory.write.setWrite16(Write16Delegate::make<MemoryBus, &MemoryBus::invalidWrite16>(this));
        mInvalidMemory.write.setWrite32(Write32Delegate::make<MemoryBus, &MemoryBus::invalidWrite32>(this));

        uint32_t pageSize = mPageLimit + 1;
        uint32_t pageCount = 1 << (mMemSizeLog2 - mPageSizeLog2);
        mReadPages.resize(pageCount);
        mWritePages.resize(pageCount);
        Page::Item pageItemRead = { 0, mPageLimit, 0, &mInvalidMemory.read };
        Page::Item pageItemWrite = { 0, mPageLimit, 0, &mInvalidMemory.write };
        for (uint32_t pageIndex = 0; pageIndex < pageCount; ++pageIndex)
        {
            mReadPages[pageIndex].mItems.push_back(pageItemRead);
            mWritePages[pageIndex].mItems.push_back(pageItemWrite);

            pageItemRead.mBase += pageSize;
            pageItemRead.mLimit += pageSize;
            pageItemRead.mOffset += pageSize;

            pageItemWrite.mBase += pageSize;
            pageItemWrite.mLimit += pageSize;
            pageItemWrite.mOffset += pageSize;
        }

        return true;
    }

    bool MemoryBus::addRange(std::vector<Page>& pageTable, uint32_t base, uint32_t size, Accessor& accessor)
    {
        EMU_VERIFY((base & 0x0f) == 0);
        EMU_VERIFY((size & 0x0f) == 0);

        uint32_t limit = base + size - 1;
        EMU_VERIFY(limit >= base);
        EMU_VERIFY(limit <= mMemLimit);

        uint32_t pageIndexLimit = limit >> mPageSizeLog2;
        uint32_t pageBase = base & ~mPageLimit;
        uint32_t pageSize = mPageLimit + 1;
        uint32_t pageLimit = pageBase + mPageLimit;
        for (uint32_t pageIndex = base >> mPageSizeLog2; pageIndex <= pageIndexLimit; ++pageIndex)
        {
            Page::Item newItem;
            newItem.mBase = std::max(base, pageBase);
            newItem.mLimit = std::min(limit, pageLimit);
            newItem.mOffset = base;
            newItem.mAccessor = &accessor;

            auto& page = pageTable[pageIndex];
            auto item = page.mItems.begin();

            // Skip items where the range is before our item
            while (item->mLimit < newItem.mBase)
                ++item;

            // Slice previous item in two at base address if applicable
            if (item->mBase < newItem.mBase)
            {
                Page::Item nextItem = *item;
                auto delta = newItem.mBase - nextItem.mBase;
                nextItem.mBase += delta;
                nextItem.mOffset += delta;

                item->mLimit = newItem.mBase - 1;
                item = page.mItems.insert(item, nextItem);
            }

            // Insert our item
            item = ++page.mItems.insert(item, newItem);

            // Now remove all items with a limit smaller than our limit
            while ((item != page.mItems.end()) && (item->mLimit <= newItem.mLimit))
                item = page.mItems.erase(item);

            // Now make sure the current item starts right after our new item limit
            if ((item != page.mItems.end()) && (item->mBase <= newItem.mLimit))
            {
                auto delta = newItem.mLimit + 1 - item->mBase;
                item->mBase += delta;
                item->mOffset += delta;
            }

            EMU_ASSERT(validatePage(page, pageBase, pageSize));

            pageBase += pageSize;
            pageLimit += pageSize;
        }
        return true;
    }

    bool MemoryBus::addRange(uint32_t base, uint32_t size, ReadAccessor& readAccessor)
    {
        return addRange(mReadPages, base, size, readAccessor);
    }

    bool MemoryBus::addRange(uint32_t base, uint32_t size, WriteAccessor& writeAccessor)
    {
        return addRange(mWritePages, base, size, writeAccessor);
    }

    bool MemoryBus::addRange(uint32_t base, uint32_t size, ReadAccessor& readAccessor, WriteAccessor& writeAccessor)
    {
        EMU_VERIFY(addRange(mReadPages, base, size, readAccessor));
        EMU_VERIFY(addRange(mWritePages, base, size, writeAccessor));
        return true;
    }

    bool MemoryBus::addRange(uint32_t base, uint32_t size, ReadWriteAccessor& readWriteAccessor)
    {
        EMU_VERIFY(addRange(mReadPages, base, size, readWriteAccessor.read));
        EMU_VERIFY(addRange(mWritePages, base, size, readWriteAccessor.write));
        return true;
    }

    MemoryBus::Page::Item* MemoryBus::findPageItem(std::vector<Page>& pageTable, uint32_t addr)
    {
        EMU_ASSERT(addr <= mMemLimit);
        uint32_t pageIndex = addr >> mPageSizeLog2;
        auto item = pageTable[pageIndex].mItems.data();
        while (addr < item->mBase)
            ++item;
        return item;
    }

    uint8_t MemoryBus::read8(uint32_t addr)
    {
        auto item = findPageItem(mReadPages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<ReadAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            return *reinterpret_cast<uint8_t*>(accessor->mMemory + addrFixed);
        }
        else
        {
            return accessor->mRead8(addr);
        }
    }

    uint16_t MemoryBus::read16(uint32_t addr)
    {
        auto item = findPageItem(mReadPages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<ReadAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            return *reinterpret_cast<uint16_t*>(accessor->mMemory + addrFixed);
        }
        else
        {
            return accessor->mRead16(addr);
        }
    }

    uint32_t MemoryBus::read32(uint32_t addr)
    {
        auto item = findPageItem(mReadPages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<ReadAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            return *reinterpret_cast<uint32_t*>(accessor->mMemory + addrFixed);
        }
        else
        {
            return accessor->mRead32(addr);
        }
    }

    void MemoryBus::write8(uint32_t addr, uint8_t value)
    {
        auto item = findPageItem(mWritePages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<WriteAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            *reinterpret_cast<uint8_t*>(accessor->mMemory + addrFixed) = value;
        }
        else
        {
            return accessor->mWrite8(addr, value);
        }
    }

    void MemoryBus::write16(uint32_t addr, uint16_t value)
    {
        auto item = findPageItem(mWritePages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<WriteAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            *reinterpret_cast<uint16_t*>(accessor->mMemory + addrFixed) = value;
        }
        else
        {
            return accessor->mWrite16(addr, value);
        }
    }

    void MemoryBus::write32(uint32_t addr, uint32_t value)
    {
        auto item = findPageItem(mWritePages, addr);
        auto addrFixed = addr - item->mOffset;
        auto accessor = static_cast<WriteAccessor*>(item->mAccessor);
        if (accessor->mMemory)
        {
            *reinterpret_cast<uint32_t*>(accessor->mMemory + addrFixed) = value;
        }
        else
        {
            return accessor->mWrite32(addr, value);
        }
    }

    uint8_t MemoryBus::invalidRead8(uint32_t addr)
    {
        printf("Invalid read8 access at address 0x%08x\n", addr);
        EMU_ASSERT(false);
        return 0;
    }

    uint16_t MemoryBus::invalidRead16(uint32_t addr)
    {
        printf("Invalid read16 access at address 0x%08x\n", addr);
        EMU_ASSERT(false);
        return 0;
    }

    uint32_t MemoryBus::invalidRead32(uint32_t addr)
    {
        printf("Invalid read32 access at address 0x%08x\n", addr);
        EMU_ASSERT(false);
        return 0;
    }

    void MemoryBus::invalidWrite8(uint32_t addr, uint8_t value)
    {
        printf("Invalid write8 access at address 0x%08x (value = 0x%02x)\n", addr, value);
        EMU_ASSERT(false);
    }

    void MemoryBus::invalidWrite16(uint32_t addr, uint16_t value)
    {
        printf("Invalid write16 access at address 0x%08x (value = 0x%04x)\n", addr, value);
        EMU_ASSERT(false);
    }

    void MemoryBus::invalidWrite32(uint32_t addr, uint32_t value)
    {
        printf("Invalid write32 access at address 0x%08x (value = 0x%08x)\n", addr, value);
        EMU_ASSERT(false);
    }

    void MemoryBus::export8(uint32_t addr, void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            static_cast<uint8_t*>(data)[index] = read8(addr);
            addr += 1;
        }
    }

    void MemoryBus::export16(uint32_t addr, void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            static_cast<uint16_t*>(data)[index] = read16(addr);
            addr += 2;
        }
    }

    void MemoryBus::export32(uint32_t addr, void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            static_cast<uint32_t*>(data)[index] = read32(addr);
            addr += 4;
        }
    }

    void MemoryBus::import8(uint32_t addr, const void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            write8(addr, static_cast<const uint8_t*>(data)[index]);
            addr += 1;
        }
    }

    void MemoryBus::import16(uint32_t addr, const void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            write16(addr, static_cast<const uint16_t*>(data)[index]);
            addr += 2;
        }
    }

    void MemoryBus::import32(uint32_t addr, const void* data, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
        {
            write32(addr, static_cast<const uint32_t*>(data)[index]);
            addr += 4;
        }
    }
}