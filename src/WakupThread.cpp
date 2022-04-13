#include "WakeupThread.h"
#include <stdint.h>
#include <mutex>
#include <thread>
#include <assert.h>

namespace wakeupthread
{

WakeupThread *gWakeupThread=nullptr;

class WakeupThreadImpl : public WakeupThread
{
public:
    WakeupThreadImpl(void)
    {
    }

    virtual ~WakeupThreadImpl(void)
    {
    }

    virtual void wakeup(void) final
    {
        mPendingWorkMutex.lock();
        mPendingWork = true;
        mPendingWorkMutex.unlock();
        mPendingWorkCond.notify_one(); // If the main thread is asleep, wake it up.
    }

    // Go to sleep until we get a 'wakeup' event from one of the other background threads or timeout was reached.
    // returns true if there is pending work from a wakeup() call
    virtual bool goToSleep(uint32_t timeoutMs) final
    {
        std::unique_lock<std::mutex> lock(mPendingWorkMutex);
        std::cv_status waitStatus = std::cv_status::no_timeout;
        while (!mPendingWork && waitStatus == std::cv_status::no_timeout)
            waitStatus = mPendingWorkCond.wait_for(lock, std::chrono::milliseconds(timeoutMs));

        bool pendingWork = mPendingWork;
        mPendingWork = false;
        return pendingWork;
    }

    virtual void release(void) final
    {
        delete this;
    }


    bool mPendingWork{ true }; // default is true, always assume one initial first frame of work
    std::mutex mPendingWorkMutex;
    std::condition_variable mPendingWorkCond;

};

WakeupThread *WakeupThread::create(void)
{
    assert(gWakeupThread == nullptr);
    auto ret = new WakeupThreadImpl;
    gWakeupThread = static_cast<WakeupThread *>(ret);
    return gWakeupThread;
}


}
