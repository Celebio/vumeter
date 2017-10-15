#include <portaudio.h>
#include "rwqueuetype.hpp"
#include <vector>
#include <experimental/optional>


class AudioInputCallbackContext;

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
    std::experimental::optional< size_t > m_inputDeviceIndex;
    std::experimental::optional< size_t > m_outputDeviceIndex;
    void displayDeviceInfo(const PaDeviceInfo *deviceInfo, int deviceIndex);
    void playTwoSmallHighPitchSine();
    void reallyListen();
    AudioInputCallbackContext createInputContext();
    PaError openInputStream(PaStream *&stream, AudioInputCallbackContext &context);
    int startStopStream(PaStream *stream);

    std::experimental::optional< size_t > findPreferedDevice(
                                                     const std::string &deviceName,
                                                     bool displayDeviceNames,
                                                     bool lookingForInputDevice,
                                                     bool lookingForOutputDevice);
};
