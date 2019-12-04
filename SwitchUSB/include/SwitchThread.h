#pragma once
#include "switch.h"

//Wrapper for switch thread implementation. Passed function will be called indefinitely until the thread closes.
class SwitchThread
{
private:
    Thread m_thread;
    bool m_isRunning = false;
    ThreadFunc m_function;
    void *m_argument;

    static void ThreadLoop(void *argument);

public:
    SwitchThread(){};

    //Constructs the class by creating and starting the thread.
    //function - the function you want to be called
    //argument - the argument passed to that function
    //stackSize - the stack size of the created thread
    //prio - thread priority, 0x00 - highest, 0x3F - lowest. Switch uses 0x2C
    SwitchThread(ThreadFunc function, void *argument, size_t stackSize, int prio);

    //SwitchThread isn't copy-movable, because the memory address of its variables is important
    SwitchThread &operator=(const SwitchThread &other) = delete;
    SwitchThread &operator=(const SwitchThread &&other) = delete;
    //Closes the thread upon exiting
    ~SwitchThread();

    //Creates the thread.
    //function - the function you want to be called
    //argument - the argument passed to that function
    //stackSize - the stack size of the created thread
    //prio - thread priority, 0x00 - highest, 0x3F - lowest. Switch uses 0x2C
    Result Initialize(size_t stackSize, int prio);

    //Starts the thread. The given function will be called with the given argument in a loop until the thread is closed.
    Result Start(ThreadFunc function, void *argument);

    //Closes the thread.
    //This will block the caller indefinitely until the thread is returned!
    Result Exit();

    bool inline IsRunning() { return m_isRunning; }
};