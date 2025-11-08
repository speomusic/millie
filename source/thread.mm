#include <pthread.h>
#include <stdexcept>
#import "merc/av/thread.h"

namespace merc::av
{
    void raiseCurrentThreadPriority()
    {
        if (pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0))
            throw std::runtime_error("Cannot set thread priority to 1.");
    }
}
