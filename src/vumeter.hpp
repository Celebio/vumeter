#ifndef VUMETER_HPP
#define VUMETER_HPP


#include "rwqueuetype.hpp"


class VuMeter {
public:
    VuMeter();
    void start();
private:
    RWQueue m_lockFreeQueue;  // lock-free queue for Audio-Gui thread communication
    RWVectorQueue m_lockFreeVectorQueue;

    void audioThreadFunction();
    void guiThreadFunction();
};

#endif
