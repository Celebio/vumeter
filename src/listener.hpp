#include <portaudio.h>
#include "rwqueuetype.hpp"
#include <vector>
#include <experimental/optional>

class AudioOutputCallbackContext;
class AudioInputCallbackContext;

class Listener {
public:
    Listener(RWQueue *lockFreeQueue,
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
    AudioInputCallbackContext createInputContextAndFillInputParameters(PaStreamParameters &inputParameters);
    PaError openInputStream(PaStream *&stream, AudioInputCallbackContext &context, PaStreamParameters &inputParameters);
    int startStopStream(PaStream *stream);

    std::experimental::optional< size_t > findPreferedDevice(
                                                     const std::string &deviceName,
                                                     bool displayDeviceNames,
                                                     bool lookingForInputDevice,
                                                     bool lookingForOutputDevice);
};
