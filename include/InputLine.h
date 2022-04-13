#pragma once

// This is a class which accepts console input in a non-blocking way that works on both Windows and Linux

#include <stdint.h>

namespace wakeupthread
{
class WakeupThread;
}


namespace inputline
{

class InputLine
{
public:
    static InputLine* create(wakeupthread::WakeupThread* wakeupThread);
    virtual const char* getInputLine(void) = 0;


    virtual void release(void) = 0;

protected:
    virtual ~InputLine(void)
    {
    }
};

}
