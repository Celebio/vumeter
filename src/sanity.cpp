#include "sanity.hpp"

void Sanity::checkNoError(PaError err){
    if (err != paNoError){
        throw int(err);
    }
}


