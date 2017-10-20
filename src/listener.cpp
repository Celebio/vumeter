#include "listener.hpp"
#include <iostream>
#include <string.h>
#include <math.h>
#include <chrono>
#include <unistd.h>
#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;
using namespace std::chrono;
using namespace std::experimental;


const int FRAMES_PER_BUFFER = 256;


class Sanity {
public:
    class PortAudioError {};
    static void checkNoError(PaError err){
        if (err != paNoError){
            throw PortAudioError();
        }
    }
};


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



template < typename StreamCallbackFunctor >
int openStreamWrapper(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *functor){
    return (static_cast< StreamCallbackFunctor * >(functor))
        ->audioCallback(inputBuffer,
                        outputBuffer,
                        framesPerBuffer,
                        timeInfo,
                        statusFlags);
}

template < typename StreamCallbackFunctor >
PaError openStream(PaStream **stream,
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



Listener::Listener(RWQueue *lockFreeQueue,
                   bool listDevices,
                   const vector< string > &preferedInputDevices,
                   const vector< string > &preferedOutputDevices) :
    m_lockFreeQueue(lockFreeQueue),
    m_portAudioResource(PortAudioResource::getInstance()),
    m_deviceFinder(listDevices, preferedInputDevices, preferedOutputDevices)
{
}

Listener::~Listener(){
}


class AudioInputCallbackContext {
public:
    PaStreamParameters inputParameters;
    RWQueue *lockFreeQueue;
    bool stereo;
    double sampleRate;
    int framesPerBuffer;
};




static int patestInputCallback( const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData )
{
    AudioInputCallbackContext *ctx = (AudioInputCallbackContext*)userData;
    float *in = (float*)inputBuffer;
    RWQueue *lockFreeQueue = ctx->lockFreeQueue;
    double avg = 0.0;

    for(unsigned long i=0; i<framesPerBuffer; i++ )
    {
        float left = *in++;
        float leftSq = left*left*256*256;
        float rightSq = 0;

        if (ctx->stereo){
            float right = *in++;
            rightSq = right*right*256*256;
            avg += (((leftSq + rightSq)/2.0));
        } else {
            avg += (leftSq);
        }
    }
    lockFreeQueue->try_enqueue((int)(avg*10.0/framesPerBuffer));

    return paContinue;
}


AudioInputCallbackContext Listener::createInputContext(){
    AudioInputCallbackContext context;

    context.inputParameters = m_deviceFinder.getInputStreamParameters();
    context.lockFreeQueue = m_lockFreeQueue;
    context.stereo = (context.inputParameters.channelCount == 2);
    context.sampleRate = 16000;     //selectedDevice->defaultSampleRate
    context.framesPerBuffer = FRAMES_PER_BUFFER;

    return context;
}

PaError Listener::openInputStream(PaStream *&stream,
                                  AudioInputCallbackContext &context){
    return Pa_OpenStream(
              &stream,
              &context.inputParameters,
              NULL,
              context.sampleRate,
              context.framesPerBuffer,
              paClipOff,
              patestInputCallback,
              &context );
}



void Listener::startStopStream(PaStream *stream) {
    Sanity::checkNoError(Pa_StartStream( stream ));
    pause();
    Sanity::checkNoError(Pa_StopStream( stream ));
    Sanity::checkNoError(Pa_CloseStream( stream ));
}

template <class SubClass >
class Streamer {
    friend int openStreamWrapper< Streamer >(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *functor);
protected:
    PaStream *m_stream;
    PaStreamParameters m_outputParameters;
    double m_sampleRate;
    int m_framesPerBuffer;

    PaError openOutputStream(){
        PaError err = openStream(&m_stream, NULL,
            &m_outputParameters,
            m_sampleRate,
            m_framesPerBuffer,
            paClipOff,
            *this);
        return err;
    }
    virtual int audioCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags) = 0;
public:
    explicit Streamer(const DeviceFinder &deviceFinder) : m_stream(nullptr) {
    }

};


class OutputStreamer : public Streamer < OutputStreamer > {
    vector<float> m_sine;
    double m_leftPhase;
    double m_rightPhase;
    double m_adjustedVelocity;
    bool m_stereo;
    high_resolution_clock::time_point m_lastTime;
    double m_lastTimeSum;
    int m_lastTimeCtr;

    int audioCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags){
        float *out = (float*)outputBuffer;
        double adjustedVelocity = m_adjustedVelocity;

        const int sinTableSize = m_sine.size();
        // cout << "sinTableSize = " << sinTableSize << endl;
        for(unsigned long i=0; i<framesPerBuffer; i++ )
        {
            *out++ = m_sine[(int)(m_leftPhase)];
            *out++ = m_sine[(int)(m_rightPhase)];

            // a funny chord
            m_leftPhase += (0.5 * (1 * adjustedVelocity) + 0.5 * (1.5 * adjustedVelocity));
            m_rightPhase += (0.5 * (1 * adjustedVelocity) + 0.5 * (2 * adjustedVelocity));

            if( m_leftPhase >= sinTableSize ) m_leftPhase -= sinTableSize;
            if( m_rightPhase >= sinTableSize ) m_rightPhase -= sinTableSize;
        }
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        duration<double, std::milli> time_span = t1 - m_lastTime;

        m_lastTime = t1;
        m_lastTimeSum += time_span.count();
        m_lastTimeCtr++;
        int nb = 5;
        if (m_lastTimeCtr>=nb){
            // only for debug, we should not have a system call inside the callback
            //cout << "Time avg : " <<  (m_lastTimeSum/(double)nb) << endl;
            m_lastTimeCtr = 0;
            m_lastTimeSum = 0;
        }

        return paContinue;
    }
public:

    explicit OutputStreamer(const DeviceFinder &deviceFinder) : Streamer(deviceFinder)
    {
        const int sinTableSize = 256;

        m_sine.resize(sinTableSize);
        for(int i=0; i<sinTableSize; i++ )
        {
            m_sine[i] = (float) 1.0 * sin( ((double)i/(double)sinTableSize) * M_PI * 2. );
        }
        m_leftPhase = m_rightPhase = 0;

        m_outputParameters = deviceFinder.getOutputStreamParameters();
        int sampleRate = 48000;  // (selectedDevice->defaultSampleRate == 16000) ? 48000 : selectedDevice->defaultSampleRate;
        m_adjustedVelocity = 6 * 44100.0 / sampleRate;
        m_stereo = (m_outputParameters.channelCount == 2);
        cout << "m_adjustedVelocity = " << m_adjustedVelocity << endl;
        m_lastTime = high_resolution_clock::now();
        m_lastTimeSum = 0.0;
        m_lastTimeCtr = 0;
        m_sampleRate = sampleRate;
        m_framesPerBuffer = FRAMES_PER_BUFFER;
    }

    void startStopStreamTwice(){
        const int duration = 180;

        Sanity::checkNoError(openOutputStream());
        Sanity::checkNoError(Pa_StartStream(m_stream));
        Pa_Sleep(duration);
        Sanity::checkNoError(Pa_StopStream(m_stream));
        Pa_Sleep( duration );
        Sanity::checkNoError(Pa_StartStream(m_stream));
        Pa_Sleep(duration);
        Sanity::checkNoError(Pa_StopStream(m_stream));
        Sanity::checkNoError(Pa_CloseStream(m_stream));
    }
};




void Listener::playTwoSmallHighPitchSine(){
    OutputStreamer(m_deviceFinder).startStopStreamTwice();
}

void Listener::reallyListen(){
    PaStream *stream = nullptr;
    AudioInputCallbackContext context = createInputContext();
    openInputStream(stream, context);
    startStopStream(stream);
}


void Listener::listenAndWrite(){
    cout << "SAV des emissions j'ecoute" << endl;
    playTwoSmallHighPitchSine();
    reallyListen();
    exit(0);
}



