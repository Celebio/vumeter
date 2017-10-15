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



Listener::Listener(RWQueue *lockFreeQueue,
                   bool listDevices,
                   const vector< string > &preferedInputDevices,
                   const vector< string > &preferedOutputDevices) :
    m_lockFreeQueue(lockFreeQueue),
    m_inputDeviceIndex(),
    m_outputDeviceIndex()
{
    int err = Pa_Initialize();
    if( err != paNoError ){
        cout << "Couldn't initialize PortAudio" << endl;
        return;
    }
    int numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 ){
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", -numDevices );
        return;
    }

    if (listDevices){
        findPreferedDevice("", true, false, false);
    }

    for (const string &preferedInputDeviceName : preferedInputDevices){
        m_inputDeviceIndex = findPreferedDevice(preferedInputDeviceName, false, true, false);
    }

    for (const string &preferedOutputDeviceName : preferedOutputDevices){
        m_outputDeviceIndex = findPreferedDevice(preferedOutputDeviceName, false, false, true);
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

Listener::~Listener(){
    Pa_Terminate();
}






class AudioOutputCallbackContext {
public:
    PaStreamParameters outputParameters;
    vector<float> sine;
    double leftPhase;
    double rightPhase;
    double adjustedVelocity;
    bool stereo;
    high_resolution_clock::time_point lastTime;
    double lastTimeSum;
    int lastTimeCtr;
    double sampleRate;
    int framesPerBuffer;
};

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

    const PaDeviceInfo *selectedDevice = Pa_GetDeviceInfo( *this->m_inputDeviceIndex );
    context.inputParameters.device = *this->m_inputDeviceIndex;
    context.inputParameters.channelCount = selectedDevice->maxInputChannels;
    context.inputParameters.sampleFormat = paFloat32;
    context.inputParameters.suggestedLatency = selectedDevice->defaultLowOutputLatency;
    context.inputParameters.hostApiSpecificStreamInfo = NULL;

    context.lockFreeQueue = this->m_lockFreeQueue;
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

int Listener::startStopStream(PaStream *stream) {
    PaError err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    pause();

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
error:
    return err;
}


class Streamer {
    AudioOutputCallbackContext m_context;
    PaStream *m_stream;

    static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
    {
        Streamer *me = (Streamer*)userData;
        return me->audioCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo,
                                 statusFlags);
    }
    int audioCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags){
        AudioOutputCallbackContext *ctx = &m_context;
        float *out = (float*)outputBuffer;
        double adjustedVelocity = ctx->adjustedVelocity;

        const int sinTableSize = ctx->sine.size();
        // cout << "sinTableSize = " << sinTableSize << endl;
        for(unsigned long i=0; i<framesPerBuffer; i++ )
        {
            *out++ = ctx->sine[(int)(ctx->leftPhase)];
            *out++ = ctx->sine[(int)(ctx->rightPhase)];

            // a funny chord
            ctx->leftPhase += (0.5 * (1 * adjustedVelocity) + 0.5 * (1.5 * adjustedVelocity));
            ctx->rightPhase += (0.5 * (1 * adjustedVelocity) + 0.5 * (2 * adjustedVelocity));

            if( ctx->leftPhase >= sinTableSize ) ctx->leftPhase -= sinTableSize;
            if( ctx->rightPhase >= sinTableSize ) ctx->rightPhase -= sinTableSize;
        }
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        duration<double, std::milli> time_span = t1 - ctx->lastTime;

        ctx->lastTime = t1;
        ctx->lastTimeSum += time_span.count();
        ctx->lastTimeCtr++;
        int nb = 5;
        if (ctx->lastTimeCtr>=nb){
            // only for debug, we should not have a system call inside the callback
            //cout << "Time avg : " <<  (ctx->lastTimeSum/(double)nb) << endl;
            ctx->lastTimeCtr = 0;
            ctx->lastTimeSum = 0;
        }

        return paContinue;
    }
    AudioOutputCallbackContext createOutputContext(int outputDeviceIndex){
        AudioOutputCallbackContext context;

        const int sinTableSize = 256;

        context.sine.resize(sinTableSize);
        for(int i=0; i<sinTableSize; i++ )
        {
            context.sine[i] = (float) 1.0 * sin( ((double)i/(double)sinTableSize) * M_PI * 2. );
        }
        context.leftPhase = context.rightPhase = 0;

        const PaDeviceInfo *selectedDevice = Pa_GetDeviceInfo( outputDeviceIndex );

        context.outputParameters.device = outputDeviceIndex;
        context.outputParameters.channelCount = selectedDevice->maxOutputChannels;
        context.outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        context.outputParameters.suggestedLatency = selectedDevice->defaultLowOutputLatency;
        context.outputParameters.hostApiSpecificStreamInfo = NULL;
        int sampleRate = (selectedDevice->defaultSampleRate == 16000) ? 48000 : selectedDevice->defaultSampleRate;
        context.adjustedVelocity = 6 * 44100.0 / sampleRate;
        context.stereo = (context.outputParameters.channelCount == 2);
        cout << "context.adjustedVelocity = " << context.adjustedVelocity << endl;
        context.lastTime = high_resolution_clock::now();
        context.lastTimeSum = 0.0;
        context.lastTimeCtr = 0;
        context.sampleRate = sampleRate;
        context.framesPerBuffer = FRAMES_PER_BUFFER;

        return context;
    }
    PaError openOutputStream(){
        PaError err = Pa_OpenStream(
            &m_stream,
            NULL, /* no input */
            &(m_context.outputParameters),
            m_context.sampleRate,
            m_context.framesPerBuffer,
            paClipOff,
            &Streamer::patestCallback,
            this );
        return err;
    }
public:
    Streamer(int outputDeviceIndex) :
        m_context(createOutputContext(outputDeviceIndex)),
        m_stream(nullptr)
    {
    }

    PaError startStopStreamTwice(){
        const int duration = 180;

        PaError err = openOutputStream();
        if( err != paNoError ) goto error;

        err = Pa_StartStream( m_stream );
        if( err != paNoError ) goto error;

        Pa_Sleep( duration );

        err = Pa_StopStream( m_stream );
        if( err != paNoError ) goto error;

        Pa_Sleep( duration );

        err = Pa_StartStream( m_stream );
        if( err != paNoError ) goto error;

        Pa_Sleep( duration );

        err = Pa_StopStream( m_stream );
        if( err != paNoError ) goto error;

        err = Pa_CloseStream( m_stream );
        if( err != paNoError ) goto error;

        error:
            return err;
    }
};

void Listener::playTwoSmallHighPitchSine(){
    Streamer(*this->m_outputDeviceIndex).startStopStreamTwice();
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


optional< size_t > Listener::findPreferedDevice(const string &preferedDeviceName,
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
        if (preferedDeviceName.size() && currentDeviceName.substr(0, preferedDeviceName.size()) == preferedDeviceName){
            return i;
        }
    }

    return optional< size_t >();
}


void Listener::displayDeviceInfo(const PaDeviceInfo *deviceInfo, int deviceIndex){
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
