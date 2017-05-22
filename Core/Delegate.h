#pragma once

#include "Core.h"
#include <algorithm>
#include <vector>

namespace emu
{
    template <typename TRet>
    class Delegate;

    template <typename TRet, typename... TArgs>
    class Delegate<TRet(TArgs...)>
    {
    public:
        Delegate()
        {
            mInstance = nullptr;
            mFunc = [](void*, TArgs...)->TRet { return TRet(); };
        }

        template <typename TRet(*TFunction)(TArgs...)>
        void bind()
        {
            mInstance = nullptr;
            mFunc = [](void*, TArgs... args)->TRet
            {
                return TFunction(args...);
            };
        }

        template <typename TClass, TRet(TClass::*TFunction)(TArgs...)>
        void bind(TClass* instance)
        {
            mInstance = instance;
            mFunc = [](void* instance, TArgs... args)->TRet
            {
                return (static_cast<TClass*>(instance)->*TFunction)(args...);
            };
        }

        template <typename TRet(*TFunction)(TArgs...)>
        static Delegate<TRet(TArgs...)> make()
        {
            Delegate<TRet(TArgs...)> item;
            item.bind<TFunction>();
            return item;
        }

        template <typename TClass, TRet(TClass::*TFunction)(TArgs...)>
        static Delegate<TRet(TArgs...)> make(TClass* instance)
        {
            Delegate<TRet(TArgs...)> item;
            item.bind<TClass, TFunction>(instance);
            return item;
        }

        TRet operator()(TArgs... args)
        {
            return mFunc(mInstance, args...);
        }

        bool operator==(const Delegate& other)
        {
            return (mInstance == other.mInstance) && (mFunc == other.mFunc);
        }

    private:
        typedef TRet(*ImplFunc)(void*, TArgs...);

        void*       mInstance;
        ImplFunc    mFunc;
    };
}