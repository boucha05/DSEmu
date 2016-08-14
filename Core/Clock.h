#pragma once

#include "Core.h"
#include <vector>

namespace emu
{
    class Clock
    {
    public:
        class IClocked
        {
        public:
            virtual int32_t execute(int32_t tick) = 0;
            virtual void advance(int32_t tick) = 0;
        };

        Clock();
        ~Clock();
        bool create();
        void execute(int32_t tick);
        void advance(int32_t tick);
        void addClocked(IClocked& clocked, bool master = false);
        void removeClocked(IClocked& clocked, bool master = false);

        int32_t getTargetTick() const
        {
            return mTargetTick;
        }

        int32_t getExecutedTick() const
        {
            return mExecutedTick;
        }

    private:
        typedef std::vector<IClocked*> ClockedList;

        ClockedList     mClockedMasters;
        ClockedList     mClockedSlaves;
        int32_t         mTargetTick;
        int32_t         mExecutedTick;
    };
}