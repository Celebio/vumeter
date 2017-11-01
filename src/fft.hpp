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
    explicit FFT(const ComplexPolynomial &p, size_t numberOfPoints);
    void setValue(size_t index, const Complex value);
    const std::vector<double> &computeFrequentialAmplitudes();
    ComplexPolynomial computeEval();
    ComplexPolynomial computeEvalInverse();

    static void displayComplexPolynomial(const ComplexPolynomial &p);
    static void displayComplexPolynomialAbs(const ComplexPolynomial &p);

private:
    size_t m_numberOfPoints;
    Complex m_omega;
    ComplexPolynomial m_coefs;
    ComplexPolynomial m_buffer;
    ComplexPolynomial m_evalResults;
    std::vector<double> m_frequentialAmplitudes;

    void fastEvalWithBuffer(size_t start, size_t step, Complex omega, size_t n, size_t outOffset, ComplexPolynomial &out, ComplexPolynomial &buffer);
};




#endif
