#include <portaudio.h>
#include <vector>
#include <experimental/optional>
#include <memory>

#include "rwqueuetype.hpp"
#include "devicefinder.hpp"

class AudioInputCallbackContext;


// Singleton that takes care of C-style resource : Pa_Initialize Pa_Terminate
class PortAudioResource {
public:
    ~PortAudioResource();
    static PortAudioResource *getInstance();
private:
    static PortAudioResource *m_instance;
    PortAudioResource();
};

class Listener {
public:
    explicit Listener(RWQueue *lockFreeQueue,
                      bool listDevices,
                      const std::vector< std::string > &preferedInputDevices,
                      const std::vector< std::string > &preferedOutputDevices);
    ~Listener();
    void listenAndWrite();
private:
    RWQueue *m_lockFreeQueue;
    std::unique_ptr<PortAudioResource> m_portAudioResource;
    DeviceFinder m_deviceFinder;
    void playTwoSmallHighPitchSine();
    void reallyListen();
    AudioInputCallbackContext createInputContext();
    PaError openInputStream(PaStream *&stream, AudioInputCallbackContext &context);
    void startStopStream(PaStream *stream);

};
