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

    void notImplemented(const char* function)
    {
        printf("Feature not implemented in %s\n", function);
    }
}
