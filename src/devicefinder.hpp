#include <portaudio.h>
#include <vector>
#include <experimental/optional>

class DeviceFinder {
public:
    explicit DeviceFinder(bool listDevices,
                          const std::vector< std::string > &preferedInputDevices,
                          const std::vector< std::string > &preferedOutputDevices);
    DeviceFinder();
    PaStreamParameters getInputStreamParameters() const;
    PaStreamParameters getOutputStreamParameters() const;
private:
    std::experimental::optional< size_t > m_inputDeviceIndex;
    std::experimental::optional< size_t > m_outputDeviceIndex;

    PaStreamParameters getStreamParameters(std::experimental::optional< size_t > deviceIndex,
                                           int PaDeviceInfo::*channelCount) const;
    void displayDeviceInfo(const PaDeviceInfo *deviceInfo, int deviceIndex);
    std::experimental::optional< size_t > findPreferedDevice(
                                                     const std::string &deviceName,
                                                     bool displayDeviceNames,
                                                     bool lookingForInputDevice,
                                                     bool lookingForOutputDevice);

};
