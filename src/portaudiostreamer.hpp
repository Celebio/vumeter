#ifndef PORTAUDIO_STREAMER_HPP
#define PORTAUDIO_STREAMER_HPP

#include <portaudio.h>
#include <experimental/optional>
#include <memory>

#include "devicefinder.hpp"


template < typename StreamCallbackFunctor >
int openStreamWrapper(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *functor){
    StreamCallbackFunctor *f = static_cast< StreamCallbackFunctor * >(functor);
    return f->audioCallback(inputBuffer,
                            outputBuffer,
                            framesPerBuffer,
                            timeInfo,
                            statusFlags);
}

template < typename StreamCallbackFunctor >
PaError openStreamCpp(PaStream **stream,
               const PaStreamParameters *inputParameters,
               const PaStreamParameters *outputParameters,
               double sampleRate,
               unsigned long framesPerBuffer,
               PaStreamFlags streamFlags,
               StreamCallbackFunctor &streamCallbackFunctor){
    return Pa_OpenStream(stream,
                         inputParameters,
                         outputParameters,
                         sampleRate,
                         framesPerBuffer,
                         streamFlags,
                         openStreamWrapper< StreamCallbackFunctor >,
                         &streamCallbackFunctor);
}

class PortAudioStreamer {
    friend int openStreamWrapper< PortAudioStreamer >(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *functor);
protected:
    PaStream *m_stream;
    std::experimental::optional<PaStreamParameters> m_inputParameters;
    std::experimental::optional<PaStreamParameters> m_outputParameters;
    double m_sampleRate;
    int m_framesPerBuffer;

    PaError openStream();
    virtual int audioCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags) = 0;
public:
    explicit PortAudioStreamer(const DeviceFinder &deviceFinder);

};

#endif

