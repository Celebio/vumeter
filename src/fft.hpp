#ifndef FFT_HPP
#define FFT_HPP

#include <vector>
#include <complex>
#include <utility>

using Complex = std::complex< double >;


class Polynomial;

class ComplexPolynomial : public std::vector< Complex > {
    using std::vector< Complex >::vector;
public:
    explicit ComplexPolynomial(const Polynomial &p);
    ComplexPolynomial operator * (const ComplexPolynomial &other);
};

class Polynomial : public std::vector< double > {
    using std::vector< double >::vector;
public:
    explicit Polynomial(const ComplexPolynomial &p);
};

class FFT {
public:
    static ComplexPolynomial eval(const ComplexPolynomial &p, int numberOfPoints);
    static ComplexPolynomial evalInverse(const ComplexPolynomial &p, int numberOfPoints);
    static void displayComplexPolynomial(const ComplexPolynomial &p);

private:
    static ComplexPolynomial eval(const ComplexPolynomial &coefs, const Complex &omega, int numberOfPoints);
    static ComplexPolynomial fastEval(const ComplexPolynomial &a, int start, int step, Complex omega, int n);
    static int adjustedNumberOfPoints(int numberOfPoints);
    static Complex computeOmega(int numberOfPoints);

    // for debug:
    static ComplexPolynomial slowEval(const ComplexPolynomial &coefs, const Complex &omega, int numberOfPoints);
};



#endif
