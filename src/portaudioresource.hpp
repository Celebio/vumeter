#ifndef DEVICE_PORTAUDIO_RESOURCE_HPP
#define DEVICE_PORTAUDIO_RESOURCE_HPP


// Singleton that takes care of C-style resource : Pa_Initialize Pa_Terminate
class PortAudioResource {
public:
    ~PortAudioResource();
    static PortAudioResource *getInstance();
private:
    static PortAudioResource *m_instance;
    PortAudioResource();
};

#endif
