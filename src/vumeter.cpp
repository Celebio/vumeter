#include "vumeter.hpp"

#include <iostream>
#include <thread>
#include <unistd.h>
#include <time.h>

#include "listener.hpp"
#include "thedisplayer.hpp"

using namespace std;

const int RQ_QUEUE_INIT_SIZE = 100;

void VuMeter::audioThreadFunction(){
    const string jabraSpeak510 = string("Jabra SPEAK 510 USB");
    Listener(&m_lockFreeQueue, true,
             {jabraSpeak510, "Built-in Microphone"},
             {jabraSpeak510, "Built-in Output"}).listenAndWrite();
}

void VuMeter::guiThreadFunction(){
    TheDisplayer(&m_lockFreeQueue).readAndDisplay();
}

VuMeter::VuMeter() :
    m_lockFreeQueue(RQ_QUEUE_INIT_SIZE){
}

void VuMeter::start(){
    thread audioThread = thread(&VuMeter::audioThreadFunction, this);

    // SDL says: "You should not expect to be able to create a window, render, or receive events on any thread other than the main one.""
    guiThreadFunction();

    audioThread.join();
}

