#include "Core.h"
#include <cassert>

namespace emu
{
    void assertValid(bool valid, const char* msg)
    {
        if (!valid)
        {
            printf("Assertion failed: %s\n", msg);
            assert(false);
        }
    }
}
