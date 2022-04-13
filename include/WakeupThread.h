#pragma once

#include <stdint.h>

namespace wakeupthread
{

class WakeupThread
{
public:
    static WakeupThread *create(void);

    virtual void wakeup(void) = 0;
    virtual bool goToSleep(uint32_t timeoutMs) = 0;
    virtual void release(void) = 0;
protected:
    virtual ~WakeupThread(void)
    {
    }
};

extern WakeupThread *gWakeupThread;

}
