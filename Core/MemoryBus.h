#pragma once

#include "Core.h"
#include "Delegate.h"
#include <vector>

namespace emu
{
    template <typename TAddrType, typename TDataType>
    class MemoryBusT
    {
    public:
        typedef typename TAddrType AddrType;
        typedef typename TDataType DataType;

        typedef Delegate<DataType(AddrType)>        ReadDelegate;
        typedef Delegate<void(AddrType, DataType)>  WriteDelegate;

        struct Accessor
        {
            Accessor& setName(const char* value)
            {
                name = value;
                return *this;
            }

            Accessor& setMemoryWritable(void* value)
            {
                readMemory = static_cast<const uint8_t*>(value);
                readFunc = ReadDelegate::make<invalidRead>();
                writeMemory = static_cast<uint8_t*>(value);
                writeFunc = WriteDelegate::make<invalidWrite>();
                return *this;
            }

            Accessor& setMemoryReadOnly(const void* value)
            {
                readMemory = static_cast<const uint8_t*>(value);
                readFunc = ReadDelegate::make<invalidRead>();
                writeMemory = nullptr;
                return *this;
            }

            Accessor& setReadFunc(ReadDelegate value)
            {
                readMemory = nullptr;
                readFunc = value;
                return *this;
            }

            Accessor& setWriteFunc(WriteDelegate value)
            {
                writeMemory = nullptr;
                writeFunc = value;
                return *this;
            }

            static DataType invalidRead(AddrType addr)
            {
                printf("Invalid read access at address 0x%08x\n", addr);
                EMU_ASSERT(false);
                return 0;
            }

            static void invalidWrite(AddrType addr, DataType value)
            {
                printf("Invalid write access at address 0x%08x (value = 0x%08x)\n", addr, value);
                EMU_ASSERT(false);
            }

            std::string         name;
            const uint8_t*      readMemory{ nullptr };
            ReadDelegate        readFunc{ ReadDelegate::make<invalidRead>() };
            uint8_t*            writeMemory{ nullptr };
            WriteDelegate       writeFunc{ WriteDelegate::make<invalidWrite>() };
        };

        struct Page
        {
            struct Item
            {
                AddrType        mBase;
                AddrType        mLimit;
                AddrType        mOffset;
                Accessor*       mAccessor;
            };

            std::vector<Item>   mItems;
        };

        bool create(AddrType memSizeLog2, AddrType pageSizeLog2)
        {
            EMU_VERIFY(memSizeLog2 <= 8 * sizeof(AddrType));
            EMU_VERIFY(pageSizeLog2 <= memSizeLog2);

            mMemSizeLog2 = memSizeLog2;
            mMemLimit = (static_cast<AddrType>(1) << memSizeLog2) - static_cast<AddrType>(1);
            mPageSizeLog2 = pageSizeLog2;
            mPageLimit = (static_cast<AddrType>(1) << pageSizeLog2) - static_cast<AddrType>(1);

            AddrType pageSize = mPageLimit + static_cast<AddrType>(1);
            AddrType pageCount = static_cast<AddrType>(1) << (mMemSizeLog2 - mPageSizeLog2);
            mPages.resize(pageCount);
            Page::Item pageItem = { 0, mPageLimit, 0, &mInvalidMemory };
            for (AddrType pageIndex = 0; pageIndex < pageCount; ++pageIndex)
            {
                mPages[pageIndex].mItems.push_back(pageItem);

                pageItem.mBase += pageSize;
                pageItem.mLimit += pageSize;
                pageItem.mOffset += pageSize;
            }

            return true;
        }

        bool addRange(AddrType base, AddrType size, Accessor& accessor)
        {
            EMU_VERIFY((base & 0x0f) == 0);
            EMU_VERIFY((size & 0x0f) == 0);

            AddrType limit = base + size - 1;
            EMU_VERIFY(limit >= base);
            EMU_VERIFY(limit <= mMemLimit);

            AddrType pageIndexLimit = limit >> mPageSizeLog2;
            AddrType pageBase = base & ~mPageLimit;
            AddrType pageSize = mPageLimit + 1;
            AddrType pageLimit = pageBase + mPageLimit;
            for (AddrType pageIndex = base >> mPageSizeLog2; pageIndex <= pageIndexLimit; ++pageIndex)
            {
                Page::Item newItem;
                newItem.mBase = std::max(base, pageBase);
                newItem.mLimit = std::min(limit, pageLimit);
                newItem.mOffset = base;
                newItem.mAccessor = &accessor;

                auto& page = mPages[pageIndex];
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

        DataType read(AddrType addr)
        {
            auto item = findPageItem(addr);
            auto addrFixed = addr - item->mOffset;
            auto accessor = item->mAccessor;
            if (accessor->readMemory)
            {
                return *reinterpret_cast<const DataType*>(accessor->readMemory + addrFixed);
            }
            else
            {
                return accessor->readFunc(addr);
            }
        }

        void write(AddrType addr, DataType value)
        {
            auto item = findPageItem(addr);
            auto addrFixed = addr - item->mOffset;
            auto accessor = item->mAccessor;
            if (accessor->writeMemory)
            {
                *reinterpret_cast<DataType*>(accessor->writeMemory + addrFixed) = value;
            }
            else
            {
                return accessor->writeFunc(addr, value);
            }
        }

        void read(AddrType addr, DataType* data, size_t count)
        {
            for (size_t index = 0; index < count; ++index)
            {
                data[index] = read(addr);
                addr += sizeof(DataType);
            }
        }

        void write(AddrType addr, const DataType* data, size_t count)
        {
            for (size_t index = 0; index < count; ++index)
            {
                write(addr, data[index]);
                addr += sizeof(DataType);
            }
        }

    private:
        typename Page::Item* findPageItem(AddrType addr)
        {
            EMU_ASSERT(addr <= mMemLimit);
            uint32_t pageIndex = addr >> mPageSizeLog2;
            auto item = mPages[pageIndex].mItems.data();
            while (addr < item->mBase)
                ++item;
            return item;
        }

        static bool validatePage(Page& page, AddrType pageBase, AddrType pageSize)
        {
            AddrType addr = pageBase;
            for (auto item : page.mItems)
            {
                EMU_VERIFY(item.mBase == addr);
                EMU_VERIFY(item.mLimit > item.mBase);
                addr = item.mLimit + 1;
            }
            EMU_VERIFY(addr == pageBase + pageSize);
            return true;
        }

        AddrType            mMemSizeLog2{ 0 };
        AddrType            mPageSizeLog2{ 0 };
        AddrType            mMemLimit{ 0 };
        AddrType            mPageLimit{ 0 };
        Accessor            mInvalidMemory;
        std::vector<Page>   mPages;
    };

    typedef MemoryBusT<uint32_t, uint32_t> MemoryBus32;
}