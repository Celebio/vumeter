#ifndef FFT_TESTER_HPP
#define FFT_TESTER_HPP

#include "fft.hpp"



class FFTTester {
public:
    void test();

    class WrongFastProductException : std::exception {};
private:
    void displayPolynomial(const Polynomial &p);
    Polynomial getTrivialProduct(const Polynomial &p1, const Polynomial &p2);
    Polynomial getFastProduct(const Polynomial &p1, const Polynomial &p2);
    bool polynomialsAreEqual(const Polynomial &p1, const Polynomial &p2);
    void testPolynomialsProduct(const Polynomial &p1, const Polynomial &p2);
    Polynomial generateRandomPolynomial();
};


#endif
