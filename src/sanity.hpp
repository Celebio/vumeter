#ifndef SANITY_HPP
#define SANITY_HPP

#include <portaudio.h>

class Sanity {
public:
    static void checkNoError(PaError err);
};

#endif
