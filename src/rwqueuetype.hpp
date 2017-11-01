#include "readerwriterqueue.h"
#include "atomicops.h"

#include <vector>

typedef moodycamel::ReaderWriterQueue<double> RWQueue;

typedef moodycamel::ReaderWriterQueue<std::vector<double>> RWVectorQueue;

