#ifndef DEVICE_SANITY_HPP
#define DEVICE_SANITY_HPP

#include <portaudio.h>

class Sanity {
public:
    static void checkNoError(PaError err);
};

#endif
