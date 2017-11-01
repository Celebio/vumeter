#include "portaudiostreamer.hpp"

using namespace std::experimental;


PaError PortAudioStreamer::openStream(){
    PaError err = openStreamCpp(&m_stream,
        m_inputParameters ? &(*m_inputParameters) : NULL,
        m_outputParameters ? &(*m_outputParameters) : NULL,
        m_sampleRate,
        m_framesPerBuffer,
        paClipOff,
        *this);
    return err;
}

PortAudioStreamer::PortAudioStreamer(const DeviceFinder &deviceFinder) :
    m_stream(nullptr),
    m_inputParameters(nullopt),
    m_outputParameters(nullopt),
    m_sampleRate(0),
    m_framesPerBuffer(0) {
}

PortAudioStreamer::PortAudioStreamer(const DeviceFinder &deviceFinder,
                                     const std::experimental::optional<PaStreamParameters> &inputParameters,
                                     const std::experimental::optional<PaStreamParameters> &outputParameters,
                                     double sampleRate,
                                     int framesPerBuffer) :
    m_stream(nullptr),
    m_inputParameters(inputParameters),
    m_outputParameters(outputParameters),
    m_sampleRate(sampleRate),
    m_framesPerBuffer(framesPerBuffer) {
}

