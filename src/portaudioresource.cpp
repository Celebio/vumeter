#include "portaudioresource.hpp"
#include "sanity.hpp"
#include <portaudio.h>

PortAudioResource* PortAudioResource::m_instance;

PortAudioResource::PortAudioResource(){
    Sanity::checkNoError(Pa_Initialize());
}
PortAudioResource::~PortAudioResource(){
    Pa_Terminate();
}
PortAudioResource *PortAudioResource::getInstance(){
    if (!PortAudioResource::m_instance)
        PortAudioResource::m_instance = new PortAudioResource();
    return PortAudioResource::m_instance;
}

