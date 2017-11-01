#include "listener.hpp"
#include "sanity.hpp"
#include "portaudiostreamer.hpp"
#include "fft.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <math.h>
#include <chrono>
#include <unistd.h>
#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;
using namespace std::chrono;
using namespace std::experimental;


const int FRAMES_PER_BUFFER = (1 << 9);
// The callback is called every FRAMES_PER_BUFFER/SampleRate :
// 512 samples / 16000 Hz = 32 ms.



class InputStreamer : public PortAudioStreamer {
    RWQueue *m_lockFreeQueue;
    RWVectorQueue *m_lockFreeVectorQueue;
    bool m_stereo;
    FFT m_fft;
    high_resolution_clock::time_point m_lastTime;

    int audioCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags){
        float *in = (float*)inputBuffer;
        double avg = 0.0;

        for(unsigned long i=0; i<framesPerBuffer; i++ )
        {
            float left = *in++;
            float leftSq = left*left*256*256;
            float rightSq = 0;

            if (m_stereo){
                float right = *in++;
                rightSq = right*right*256*256;
                avg += (((leftSq + rightSq)/2.0));
            } else {
                avg += (leftSq);
            }
            m_fft.setValue(i, Complex(left, 0.0));
            // m_fftTime[i] = Complex(left, 0.0);
        }
        m_lockFreeQueue->try_enqueue((int)(avg*10.0/framesPerBuffer));
        m_lockFreeVectorQueue->try_enqueue(m_fft.computeFrequentialAmplitudes());

        // high_resolution_clock::time_point t1 = high_resolution_clock::now();
        // duration<double, std::milli> time_span = t1 - m_lastTime;
        // m_lastTime = t1;
        // cout << "Elapsed: " << time_span.count() << endl;
        // cout << "current " << (double)timeInfo->currentTime/1.0 << endl;

        return paContinue;
    }
public:
    explicit InputStreamer(const DeviceFinder &deviceFinder,
                           RWQueue *lockFreeQueue,
                           RWVectorQueue *lockFreeVectorQueue) :
        PortAudioStreamer(deviceFinder,
                          deviceFinder.getInputStreamParameters(),
                          nullopt,
                          16000,
                          FRAMES_PER_BUFFER),
        m_lockFreeQueue(lockFreeQueue),
        m_lockFreeVectorQueue(lockFreeVectorQueue),
        m_stereo(m_inputParameters->channelCount == 2),
        m_fft(ComplexPolynomial(m_framesPerBuffer), m_framesPerBuffer),
        m_lastTime()
    {}

    void waitForever(){
        Sanity::checkNoError(openStream());
        Sanity::checkNoError(Pa_StartStream(m_stream));
        pause();
        Sanity::checkNoError(Pa_StopStream(m_stream));
        Sanity::checkNoError(Pa_CloseStream(m_stream));
    }
};


class SineOutputStreamer : public PortAudioStreamer {
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

    explicit SineOutputStreamer(const DeviceFinder &deviceFinder) :
        PortAudioStreamer(deviceFinder),
        m_sine(),
        m_leftPhase(0),
        m_rightPhase(0),
        m_adjustedVelocity(),
        m_stereo(),
        m_lastTime(high_resolution_clock::now()),
        m_lastTimeSum(0.0),
        m_lastTimeCtr(0)
    {
        const int sinTableSize = 256;

        m_sine.resize(sinTableSize);
        for(int i=0; i<sinTableSize; i++ )
        {
            m_sine[i] = (float) 0.2 * sin( ((double)i/(double)sinTableSize) * M_PI * 2. );
        }

        m_outputParameters = deviceFinder.getOutputStreamParameters();
        int sampleRate = 48000;
        m_adjustedVelocity = 6 * 44100.0 / sampleRate;
        m_stereo = (m_outputParameters->channelCount == 2);
        // cout << "m_adjustedVelocity = " << m_adjustedVelocity << endl;
        m_sampleRate = sampleRate;
        m_framesPerBuffer = FRAMES_PER_BUFFER;
    }

    void startStopStreamTwice(){
        const int duration = 180;

        Sanity::checkNoError(openStream());
        Sanity::checkNoError(Pa_StartStream(m_stream));
        Pa_Sleep(duration);
        Sanity::checkNoError(Pa_StopStream(m_stream));
        Pa_Sleep(duration);
        Sanity::checkNoError(Pa_StartStream(m_stream));
        Pa_Sleep(duration);
        Sanity::checkNoError(Pa_StopStream(m_stream));
        Sanity::checkNoError(Pa_CloseStream(m_stream));
    }
};



Listener::Listener(RWQueue *lockFreeQueue,
                   RWVectorQueue *lockFreeVectorQueue,
                   bool listDevices,
                   const vector< string > &preferedInputDevices,
                   const vector< string > &preferedOutputDevices) :
    m_lockFreeQueue(lockFreeQueue),
    m_lockFreeVectorQueue(lockFreeVectorQueue),
    m_portAudioResource(PortAudioResource::getInstance()),
    m_deviceFinder(listDevices, preferedInputDevices, preferedOutputDevices)
{
}

Listener::~Listener(){
}

void Listener::playTwoSmallHighPitchSine(){
    SineOutputStreamer(m_deviceFinder).startStopStreamTwice();
}

void Listener::reallyListen(){
    InputStreamer(m_deviceFinder, m_lockFreeQueue, m_lockFreeVectorQueue).waitForever();
}

void Listener::listenAndWrite(){
    cout << "SAV des emissions j'ecoute" << endl;
    playTwoSmallHighPitchSine();
    reallyListen();
    exit(0);
}



