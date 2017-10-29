#include "fft.hpp"
#include <iostream>
#include <algorithm>

using namespace std;


bool isPowerOfTwo(int val){
    return ((val & (val-1)) == 0);
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
    int i = 0;
    for (const auto &v : p){
        cout << "\t" << i++ << " \t: " << v.real() << " +  i * " << v.imag() << endl;
    }
}

int FFT::adjustedNumberOfPoints(int numberOfPoints){
    while (!isPowerOfTwo(numberOfPoints)){
        numberOfPoints++;
    }
    return numberOfPoints;
}

Complex FFT::computeOmega(int numberOfPoints){
    static const double pi = std::acos(-1);

    return exp(2i * pi / (double)numberOfPoints);
}

void FFT::fastEvalWithBuffer(const ComplexPolynomial &a, size_t start, size_t step, Complex omega, size_t n, size_t outOffset, ComplexPolynomial &out, ComplexPolynomial &buffer){
    if (n == 1){
        Complex val;
        if (start < a.size()){
            val = a[start];
        }
        out[outOffset] = val;
        return;
    }

    FFT::fastEvalWithBuffer(a, start, step*2, pow(omega, 2), n/2, outOffset, out, buffer);
    FFT::fastEvalWithBuffer(a, start+step, step*2, pow(omega, 2), n/2, outOffset+n/2, out, buffer);

    for (size_t i=0; i<n/2; i++){
        buffer[i] = out[outOffset+i] + pow(omega, i) * out[outOffset+n/2+i];
        buffer[i+n/2] = out[outOffset+i] - pow(omega, i) * out[outOffset+n/2+i];
        // the same as :
        //   buffer[i+n/2] = out[outOffset+i] - pow(omega, i+n/2) * out[outOffset+n/2+i];
        // see : https://imgur.com/ZAPGXJ9
    }
    for (size_t i=0; i<n; i++){
        out[outOffset+i] = buffer[i];
    }
}

ComplexPolynomial FFT::eval(const ComplexPolynomial &coefs,
                            const Complex &omega,
                            int numberOfPoints){
    ComplexPolynomial c(numberOfPoints);
    ComplexPolynomial buffer(numberOfPoints);

    FFT::fastEvalWithBuffer(coefs, 0, 1, omega, numberOfPoints, 0, c, buffer);
    return c;

    // return FFT::fastEvalWithAllocation(coefs, 0, 1, omega, numberOfPoints);
}

ComplexPolynomial FFT::eval(const ComplexPolynomial &coefs, int numberOfPoints){
    numberOfPoints = FFT::adjustedNumberOfPoints(numberOfPoints);
    const Complex omega = FFT::computeOmega(numberOfPoints);

    // cout << "n = " << numberOfPoints << endl;
    // cout << "omega = " << omega << endl;

    return FFT::eval(coefs, omega, numberOfPoints);
}

ComplexPolynomial FFT::evalInverse(const ComplexPolynomial &coefs, int numberOfPoints){
    numberOfPoints = FFT::adjustedNumberOfPoints(numberOfPoints);
    const Complex omega = pow(FFT::computeOmega(numberOfPoints), -1);

    // cout << "n = " << numberOfPoints << endl;
    // cout << "omega = " << omega << endl;

    ComplexPolynomial result = FFT::eval(coefs, omega, numberOfPoints);
    transform(result.begin(), result.end(), result.begin(), [numberOfPoints](const Complex &c){
        return c/((double)numberOfPoints);
    });

    return result;
}

// Use for debug :

ComplexPolynomial FFT::slowEval(const ComplexPolynomial &coefs,
                            const Complex &omega,
                            int numberOfPoints){
    ComplexPolynomial result(numberOfPoints, Complex(0.0, 0.0));

    for (size_t i=0; i<result.size(); i++){
        for (size_t j=0; j<result.size() && j<coefs.size(); j++){
            result[i] += coefs[j] * pow(omega, i*j);
        }
    }

    return result;
}

ComplexPolynomial FFT::fastEvalWithAllocation(const ComplexPolynomial &coefs, size_t start, size_t step, Complex omega, size_t n){
    ComplexPolynomial evaluated = {};
    if (n == 1){
        Complex val;
        if (start < coefs.size()){
            val = coefs[start];
        }
        evaluated.push_back(val);
        return evaluated;
    }
    evaluated.resize(n);

    ComplexPolynomial coefsEvenEval = FFT::fastEvalWithAllocation(coefs, start, step*2, pow(omega, 2), n/2);
    ComplexPolynomial coefsOddEval = FFT::fastEvalWithAllocation(coefs, start+step, step*2, pow(omega, 2), n/2);

    for (size_t i=0; i<n/2; i++){
        evaluated[i] = coefsEvenEval[i] + pow(omega, i) * coefsOddEval[i];
        evaluated[i+n/2] = coefsEvenEval[i] - pow(omega, i) * coefsOddEval[i];
        // the same as :
        //   evaluated[i+n/2] = coefsEvenEval[i] + pow(omega, (i+n/2)) * coefsOddEval[i];
        // see : https://imgur.com/ZAPGXJ9
    }

    return evaluated;
}


