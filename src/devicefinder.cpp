#include "devicefinder.hpp"
#include <iostream>

using namespace std;
using namespace std::experimental;


DeviceFinder::DeviceFinder() :
    m_inputDeviceIndex(nullopt),
    m_outputDeviceIndex(nullopt)
{
}

 DeviceFinder::DeviceFinder(bool listDevices,
                            const std::vector< std::string > &preferedInputDevices,
                            const std::vector< std::string > &preferedOutputDevices){

    int numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 ){
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", -numDevices );
        return;
    }

    if (listDevices){
        findPreferedDevice("", true, false, false);
    }

    for (const string &preferedInputDeviceName : preferedInputDevices){
        auto deviceIndex = findPreferedDevice(preferedInputDeviceName, false, true, false);
        if (deviceIndex){
            m_inputDeviceIndex = deviceIndex;
        }
    }

    for (const string &preferedOutputDeviceName : preferedOutputDevices){
        auto deviceIndex = findPreferedDevice(preferedOutputDeviceName, false, false, true);
        if (deviceIndex){
            m_outputDeviceIndex = deviceIndex;
        }
    }

    if (this->m_inputDeviceIndex){
        cout << endl << "Selected input device no : " << *this->m_inputDeviceIndex << endl;
        displayDeviceInfo(Pa_GetDeviceInfo(*this->m_inputDeviceIndex), *this->m_inputDeviceIndex);
    }
    if (this->m_outputDeviceIndex){
        cout << endl << "Selected output device no : " << *this->m_outputDeviceIndex << endl;
        displayDeviceInfo(Pa_GetDeviceInfo(*this->m_outputDeviceIndex), *this->m_outputDeviceIndex);
    }
}

PaStreamParameters DeviceFinder::getInputStreamParameters() const {
    return getStreamParameters(m_inputDeviceIndex, &PaDeviceInfo::maxInputChannels);
}
PaStreamParameters DeviceFinder::getOutputStreamParameters() const {
    return getStreamParameters(m_outputDeviceIndex, &PaDeviceInfo::maxOutputChannels);
}

PaStreamParameters DeviceFinder::getStreamParameters(optional< size_t > deviceIndex, int PaDeviceInfo::*channelCount) const {
    PaStreamParameters streamParameters;

    const PaDeviceInfo *selectedDevice = Pa_GetDeviceInfo( *deviceIndex );
    streamParameters.device = *deviceIndex;
    streamParameters.channelCount = selectedDevice->*channelCount;
    streamParameters.sampleFormat = paFloat32;
    streamParameters.suggestedLatency = selectedDevice->defaultLowOutputLatency;
    streamParameters.hostApiSpecificStreamInfo = NULL;

    return streamParameters;
}



optional< size_t > DeviceFinder::findPreferedDevice(const string &preferedDeviceName,
                                                 bool displayDeviceNames,
                                                 bool lookingForInputDevice,
                                                 bool lookingForOutputDevice){
    int numDevices = Pa_GetDeviceCount();
    for(int i=0; i<numDevices; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (displayDeviceNames){
            displayDeviceInfo(deviceInfo, i);
        }

        const string currentDeviceName = string(deviceInfo->name);

        if (preferedDeviceName.size() &&
            currentDeviceName.substr(0, preferedDeviceName.size()) == preferedDeviceName &&
            ((lookingForInputDevice && deviceInfo->maxInputChannels) ||
             (lookingForOutputDevice && deviceInfo->maxOutputChannels))
           ){
            return i;
        }
    }

    return optional< size_t >();
}


void DeviceFinder::displayDeviceInfo(const PaDeviceInfo *deviceInfo, int deviceIndex){
    cout << "-----------------------------------" << endl;
    cout << "Device : " << deviceIndex << endl;
    cout << "\t Name : \t" << deviceInfo->name << endl;
    cout << "\t hostApi : \t" << deviceInfo->hostApi << endl;
    cout << "\t maxInputChannels : \t" << deviceInfo->maxInputChannels << endl;
    cout << "\t maxOutputChannels : \t" << deviceInfo->maxOutputChannels << endl;

    cout << "\t defaultLowInputLatency : \t" << deviceInfo->defaultLowInputLatency << endl;
    cout << "\t defaultLowOutputLatency : \t" << deviceInfo->defaultLowOutputLatency << endl;
    cout << "\t defaultHighInputLatency : \t" << deviceInfo->defaultHighInputLatency << endl;
    cout << "\t defaultHighOutputLatency : \t" << deviceInfo->defaultHighOutputLatency << endl;
    cout << "\t defaultSampleRate : \t" << deviceInfo->defaultSampleRate << endl;
    cout << "-----------------------------------" << endl;
}