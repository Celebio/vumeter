#include "fft.hpp"
#include <iostream>
#include <algorithm>

using namespace std;


bool isPowerOfTwo(size_t val){
    return ((val & (val-1)) == 0);
}

size_t adjustedNumberOfPoints(size_t numberOfPoints){
    while (!isPowerOfTwo(numberOfPoints)){
        numberOfPoints++;
    }
    return numberOfPoints;
}

Complex computeOmega(size_t numberOfPoints){
    static const double pi = std::acos(-1);

    return exp(2i * pi / (double)numberOfPoints);
}

ComplexPolynomial::ComplexPolynomial(const Polynomial &p){
    for (const auto &realCoef : p){
        this->push_back(Complex(realCoef, 0.0));
    }
}

ComplexPolynomial ComplexPolynomial::operator * (const ComplexPolynomial &other){
    ComplexPolynomial result = {};

    transform(this->begin(), this->end(), other.begin(), back_inserter(result),
        [](const Complex &c1, const Complex &c2){
            return c1*c2;
        });
    return result;
}

Polynomial::Polynomial(const ComplexPolynomial &p){
    for (const auto &complexCoef : p){
        this->push_back(complexCoef.real());
    }
}

void FFT::displayComplexPolynomial(const ComplexPolynomial &p){
    int ctr = 0;
    for (const auto &v : p){
        cout << "\t" << ctr++ << " \t: " << v.real() << " +  i * " << v.imag() << endl;
    }
}
void FFT::displayComplexPolynomialAbs(const ComplexPolynomial &p){
    int ctr = 0;
    for (const auto &v : p){
        cout << "\t" << ctr++ << " \t: " << abs(v) << endl;
    }
}



FFT::FFT(const ComplexPolynomial &p, size_t numberOfPoints) :
    m_numberOfPoints(adjustedNumberOfPoints(numberOfPoints)),
    m_omega(computeOmega(m_numberOfPoints)),
    m_coefs(p),
    m_buffer(ComplexPolynomial(m_numberOfPoints)),
    m_evalResults(ComplexPolynomial(m_numberOfPoints)),
    m_frequentialAmplitudes(m_numberOfPoints)
{
    m_coefs.resize(m_numberOfPoints, Complex(0, 0));
}

void FFT::setValue(size_t index, const Complex value){
    m_coefs[index] = value;
}

const vector<double> &FFT::computeFrequentialAmplitudes() {
    fastEvalWithBuffer(0,
                       1,
                       m_omega,
                       m_numberOfPoints,
                       0,
                       m_evalResults,
                       m_buffer);
    transform(m_evalResults.begin(), m_evalResults.end(), m_frequentialAmplitudes.begin(), [this](const Complex &c){
        return abs(c);
    });

    return m_frequentialAmplitudes;
}

ComplexPolynomial FFT::computeEval(){
    fastEvalWithBuffer(0,
                       1,
                       m_omega,
                       m_numberOfPoints,
                       0,
                       m_evalResults,
                       m_buffer);
    return m_evalResults;
}
ComplexPolynomial FFT::computeEvalInverse(){
    fastEvalWithBuffer(0,
                       1,
                       pow(m_omega, -1),
                       m_numberOfPoints,
                       0,
                       m_evalResults,
                       m_buffer);
    transform(m_evalResults.begin(), m_evalResults.end(), m_evalResults.begin(), [this](const Complex &c){
        return c/((double)this->m_numberOfPoints);
    });
    return m_evalResults;
}



void FFT::fastEvalWithBuffer(size_t start, size_t step, Complex omega, size_t n, size_t outOffset, ComplexPolynomial &out, ComplexPolynomial &buffer){
    if (n == 1){
        Complex val;
        if (start < m_coefs.size()){
            val = m_coefs[start];
        }
        out[outOffset] = val;
        return;
    }

    fastEvalWithBuffer(start, step*2, pow(omega, 2), n/2, outOffset, out, buffer);
    fastEvalWithBuffer(start+step, step*2, pow(omega, 2), n/2, outOffset+n/2, out, buffer);

    for (size_t i=0; i<n/2; i++){
        buffer[i] = out[outOffset+i] + pow(omega, i) * out[outOffset+n/2+i];
        buffer[i+n/2] = out[outOffset+i] - pow(omega, i) * out[outOffset+n/2+i];
        // the same as :
        //   buffer[i+n/2] = out[outOffset+i] - pow(omega, i+n/2) * out[outOffset+n/2+i];
        // see : https://imgur.com/ZAPGXJ9
    }

    memcpy(&out[outOffset], &buffer[0], n*sizeof(Complex));
}



