#include "Clock.h"

namespace emu
{
    Clock::Clock()
        : mTargetTick(0)
        , mExecutedTick(0)
    {
    }

    Clock::~Clock()
    {
    }

    bool Clock::create()
    {
        return true;
    }

    void Clock::execute(int32_t tick)
    {
        while (getExecutedTick() < tick)
        {
            // Advance masters until a desired target tick is reached
            mTargetTick = tick;
            while (getExecutedTick() < mTargetTick)
            {
                int32_t masterTick = mTargetTick;
                for (auto clocked : mClockedMasters)
                {
                    int32_t executedTick = clocked->execute(mTargetTick);
                    masterTick = std::min(masterTick, executedTick);
                }
                mExecutedTick = masterTick;

                // TODO: Target tick should be the minimum from a list of scheduled operations
                mTargetTick = masterTick;
            }

            // Advance slaves to the target tick
            for (auto clocked : mClockedSlaves)
            {
                clocked->execute(mTargetTick);
            }
        }
    }

    void Clock::advance(int32_t tick)
    {
        for (auto clocked : mClockedMasters)
            clocked->advance(tick);
        for (auto clocked : mClockedSlaves)
            clocked->advance(tick);
    }

    void Clock::addClocked(IClocked& clocked, bool master)
    {
        auto& list = master ? mClockedMasters : mClockedSlaves;
        list.push_back(&clocked);
    }

    void Clock::removeClocked(IClocked& clocked, bool master)
    {
        auto& list = master ? mClockedMasters : mClockedSlaves;
        auto item = std::find(list.begin(), list.end(), &clocked);
        list.erase(item);
    }
}
