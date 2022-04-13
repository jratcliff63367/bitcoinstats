#include "InputLine.h"
#include "WakeupThread.h"
#include "wplatform.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

#define MAX_INPUT_BUF 8192

#if CARB_COMPILER_MSC
#    pragma warning(disable : 4996)
#endif

#define SLEEP_NANOSECONDS 10000000

namespace inputline
{

class InputLineImpl : public InputLine
{
public:
    InputLineImpl(wakeupthread::WakeupThread* wakeupThread) : mWakeupThread(wakeupThread)
    {
        mThread = new std::thread([this]() 
        {
            while (!mExit)
            {
                // Not allowed to start inputting a new line of data
                if (!mHaveNewData)
                {
                    mInputBuffer[0] = 0;
                    char* ret = fgets(mInputBuffer, sizeof(mInputBuffer), stdin);
                    const auto len = strlen(mInputBuffer);
                    if (ret && len)
                    {
                        if (mWakeupThread)
                        {
                            mWakeupThread->wakeup();
                        }
                        mHaveNewData = true;
                    }
                    else if (!ret)
                    {
                        // on EOF or input error exit this loop
                        printf("InputLineImpl: stopping the read loop, encountered EOF or error in the input stream\n");
                        break;
                    }
                }
                else
                {
                    wplatform::sleepNano(SLEEP_NANOSECONDS);
                }
            }
        });
    }

    virtual ~InputLineImpl(void)
    {
        if (mThread)
        {
            mExit = true;
            mThread->detach();
            delete mThread;
        }
    }

    virtual const char* getInputLine(void) override final
    {
        const char* ret = nullptr;
        if (mHaveNewData)
        {
            lock();
            strcpy(mResultBuffer, mInputBuffer);
            size_t len = strlen(mResultBuffer);
            if (len && mResultBuffer[len - 1] == 0x0A)
            {
                mResultBuffer[len - 1] = 0;
            }
            mHaveNewData = false;
            unlock();
            ret = mResultBuffer;
        }

        return ret;
    }

    virtual void release(void) override final
    {
        delete this;
    }


    void lock(void)
    {
        mMutex.lock();
    }

    void unlock(void)
    {
        mMutex.unlock();
    }


protected:
    bool mSetServerSocket{ false };
    bool mSetClientSocket{ false };
    char mInputBuffer[MAX_INPUT_BUF];
    char mResultBuffer[MAX_INPUT_BUF];
    std::thread* mThread{ nullptr }; // Worker thread to get console input
    std::mutex mMutex;
    std::atomic<bool> mExit{ false };
    std::atomic<bool> mHaveNewData{ false };
    wakeupthread::WakeupThread* mWakeupThread{ nullptr };
};

InputLine* InputLine::create(wakeupthread::WakeupThread* wakeupThread)
{
    auto ret = new InputLineImpl(wakeupThread);
    return static_cast<InputLine*>(ret);
}


}
