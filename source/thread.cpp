#include "merc/windows-utils.h"
#include "merc/thread.h"

namespace merc::av
{
    void raiseCurrentThreadPriority()
    {
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
            THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
    }
}
