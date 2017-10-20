#ifndef DEVICE_VUMETER_HPP
#define DEVICE_VUMETER_HPP


#include "rwqueuetype.hpp"


class VuMeter {
public:
    VuMeter();
    void start();
private:
    RWQueue m_lockFreeQueue;  // lock-free queue for Audio-Gui thread communication

    void audioThreadFunction();
    void guiThreadFunction();
};

#endif
