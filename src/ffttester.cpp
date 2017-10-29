#include "ffttester.hpp"

#include <iostream>
#include <algorithm>
#include <list>
#include <numeric>
#include <random>
#include <vector>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <stdlib.h>

using namespace std;

void FFTTester::testPolynomialsProduct(const Polynomial &p1, const Polynomial &p2){
    Polynomial p3 = getTrivialProduct(p1, p2);
    Polynomial fastP3 = getFastProduct(p1, p2);

    // cout << endl << "p3 : " << endl;
    // displayPolynomial(p3);
    // cout << endl << "fastP3 : " << endl;
    // displayPolynomial(fastP3);

    if (!polynomialsAreEqual(p3, fastP3)){
        throw WrongFastProductException();
    }

    cout << "Test OK: 2 polynomials of size : " << p1.size() << " and " << p2.size() << endl;
}

Polynomial FFTTester::generateRandomPolynomial(){
    int size = rand() % 8000;
    Polynomial result(size, 0.0);

    for (auto &c : result){
        c = ((rand() % 2000)-1000)/10.0;
    }

    return result;
}

void FFTTester::test(){
    srand(3);
    std::setprecision(2);

    {
        Polynomial p1 = { 2.0, 5.6, 7.0, 6.3853, 9.0, 0, 3.0, 5.6, 2.6 };
        Polynomial p2 = { 5.9, 23.5, 6.283, 9.3, 12.4, 5.1, 1.1, 0.3, 0, 4.0 };

        testPolynomialsProduct(p1, p2);
    }

    {
        for (int i=0; i<100; i++){
            Polynomial p1 = generateRandomPolynomial();
            Polynomial p2 = generateRandomPolynomial();

            testPolynomialsProduct(p1, p2);
        }
    }
}

bool FFTTester::polynomialsAreEqual(const Polynomial &p1, const Polynomial &p2){
    for (size_t i=0; i<max(p1.size(), p2.size()); i++){
        if (abs((i < p1.size() ? p1[i] : 0) - (i < p2.size() ? p2[i] : 0)) > 0.00001){
            cout << "Different ! " << p1[i] << " vs " << p2[i] << endl;
            return false;
        }
    }
    return true;
}

Polynomial FFTTester::getFastProduct(const Polynomial &p1, const Polynomial &p2){
    int n = p1.size();
    int m = p2.size();
    int k = (n-1)+(m-1)+1;

    ComplexPolynomial c1Eval = FFT::eval(ComplexPolynomial(p1), k);
    ComplexPolynomial c2Eval = FFT::eval(ComplexPolynomial(p2), k);
    ComplexPolynomial c3Eval = c1Eval * c2Eval;

    ComplexPolynomial c3 = FFT::evalInverse(c3Eval, k);

    Polynomial res(FFT::evalInverse(c3Eval, k));
    return res;
}

Polynomial FFTTester::getTrivialProduct(const Polynomial &p1, const Polynomial &p2){
    size_t n = p1.size();
    size_t m = p2.size();

    Polynomial p3((n-1)+(m-1)+1, 0);

    for (size_t i=0; i<p3.size(); i++){
        double val = 0;
        for (size_t j=0; j<=i; j++){
            val += (j < n ? (p1[j]) : 0 ) * (((i-j) >= 0 && (i-j)<m) ? (p2[i-j]) : 0);
        }
        p3[i] = val;
    }

    return p3;
}

void FFTTester::displayPolynomial(const Polynomial &p){
    int i = 0;
    for (const auto &v : p){
        cout << "\t" << i++ << " \t: " << v << endl;
    }
}

