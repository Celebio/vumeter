#include <vector>
#include <experimental/optional>

class DeviceFinder {
public:
    explicit DeviceFinder(const std::vector< std::string > &preferedInputDevices,
                          const std::vector< std::string > &preferedOutputDevices);
    DeviceFinder();
private:
    std::experimental::optional< size_t > m_inputDeviceIndex;
    std::experimental::optional< size_t > m_outputDeviceIndex;

};
