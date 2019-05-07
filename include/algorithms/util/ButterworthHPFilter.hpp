#pragma once
#include <cassert>
#include <cmath>

namespace fluid {
namespace algorithm {

class ButterworthHPFilter {
public:
  void init(double cutoff) { // as fraction of sample rate
    using std::pow;
    double c = tan(M_PI * cutoff);
    mB0 = 1.0 / (1.0 + M_SQRT2 * c + pow(c, 2.0));
    mB1 = -2.0 * mB0;
    mB2 = mB0;
    mA0 = 2.0 * mB0 * (pow(c, 2.0) - 1.0);
    mA1 = mB0 * (1.0 - M_SQRT2 * c + pow(c, 2.0));
  }

  double processSample(double x) {
    double y = mB0 * x + mB1 * mXnz1 + mB2 * mXnz2 - mA0 * mYnz1 - mA1 * mYnz2;
    mXnz2 = mXnz1;
    mXnz1 = x;
    mYnz2 = mYnz1;
    mYnz1 = y;
    return y;
  }

private:
  double mB0, mB1, mB2;
  double mA0, mA1;
  double mXnz1 = 0;
  double mXnz2 = 0;
  double mYnz1 = 0;
  double mYnz2 = 0;
};
}; // namespace algorithm
}; // namespace fluid
