#include "vumeter.hpp"
#include <signal.h>
#include <iostream>

void signalHandler(int s){
    std::cout << "Caught signal " << s << std::endl;
    exit(1);
}

int main(int argc, char *argv[]){
    // raise(SIGSTOP);

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = signalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    VuMeter().start();
}


