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


const int FRAMES_PER_BUFFER = 256;
const int TABLE_SIZE = 256;




class AudioOutputCallbackContext {
public:
    float sine[TABLE_SIZE];
    double left_phase;
    double right_phase;
    double adjustedVelocity;
    bool stereo;
    high_resolution_clock::time_point lastTime;
    double lastTimeSum;
    int lastTimeCtr;
    double sampleRate;
};

class AudioInputCallbackContext {
public:
    RWQueue *lockFreeQueue;
    bool stereo;
    double sampleRate;
};




static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    AudioOutputCallbackContext *ctx = (AudioOutputCallbackContext*)userData;
    float *out = (float*)outputBuffer;
    double adjustedVelocity = ctx->adjustedVelocity;

    for(unsigned long i=0; i<framesPerBuffer; i++ )
    {
        *out++ = ctx->sine[(int)(ctx->left_phase)];
        *out++ = ctx->sine[(int)(ctx->right_phase)];

        // a funny chord
        ctx->left_phase += (0.5 * (1 * adjustedVelocity) + 0.5 * (1.5 * adjustedVelocity));
        ctx->right_phase += (0.5 * (1 * adjustedVelocity) + 0.5 * (2 * adjustedVelocity));

        if( ctx->left_phase >= TABLE_SIZE ) ctx->left_phase -= TABLE_SIZE;
        if( ctx->right_phase >= TABLE_SIZE ) ctx->right_phase -= TABLE_SIZE;
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


void Listener::playTwoSmallHighPitchSine(){
    const int duration = 180;

    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    AudioOutputCallbackContext data;

    for(int i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) 1.0 * sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;

    const PaDeviceInfo *selectedDevice = Pa_GetDeviceInfo( *this->m_outputDeviceIndex );

    outputParameters.device = *this->m_outputDeviceIndex;
    outputParameters.channelCount = selectedDevice->maxOutputChannels;
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = selectedDevice->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    int sampleRate = (selectedDevice->defaultSampleRate == 16000) ? 48000 : selectedDevice->defaultSampleRate;
    data.adjustedVelocity = 6 * 44100.0 / sampleRate;
    data.stereo = (outputParameters.channelCount == 2);
    cout << "data.adjustedVelocity = " << data.adjustedVelocity << endl;
    data.lastTime = high_resolution_clock::now();
    data.lastTimeSum = 0.0;
    data.lastTimeCtr = 0;
    data.sampleRate = sampleRate;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              sampleRate,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;


    Pa_Sleep( duration );

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    Pa_Sleep( duration );

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    Pa_Sleep( duration );

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

error: ;
}



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


AudioInputCallbackContext Listener::createInputContextAndFillInputParameters(PaStreamParameters &inputParameters){
    AudioInputCallbackContext context;

    const PaDeviceInfo *selectedDevice = Pa_GetDeviceInfo( *this->m_inputDeviceIndex );
    inputParameters.device = *this->m_inputDeviceIndex;
    inputParameters.channelCount = selectedDevice->maxInputChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = selectedDevice->defaultLowOutputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    context.lockFreeQueue = this->m_lockFreeQueue;
    context.stereo = (inputParameters.channelCount == 2);
    context.sampleRate = 16000;     //selectedDevice->defaultSampleRate

    return context;
}

PaError Listener::openInputStream(PaStream *&stream,
                                  AudioInputCallbackContext &context,
                                  PaStreamParameters &inputParameters){
    return Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,
              context.sampleRate,
              FRAMES_PER_BUFFER,
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


void Listener::reallyListen(){
    PaStreamParameters inputParameters;
    PaStream *stream = nullptr;
    AudioInputCallbackContext context = createInputContextAndFillInputParameters(inputParameters);
    openInputStream(stream, context, inputParameters);
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
