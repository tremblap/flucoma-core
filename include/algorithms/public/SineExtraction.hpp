/*
Part of the Fluid Corpus Manipulation Project (http://www.flucoma.org/)
Copyright 2017-2019 University of Huddersfield.
Licensed under the BSD-3 License.
See license.md file in the project root for full license information.
This project has received funding from the European Research Council (ERC)
under the European Union’s Horizon 2020 research and innovation programme
(grant agreement No 725899).
*/
#pragma once

#include "WindowFuncs.hpp"
#include "../util/AlgorithmUtils.hpp"
#include "../util/ConvolutionTools.hpp"
#include "../util/FFT.hpp"
#include "../util/FluidEigenMappings.hpp"
#include "../util/PartialTracking.hpp"
#include "../util/PeakDetection.hpp"
#include "../../data/FluidIndex.hpp"
#include "../../data/TensorTypes.hpp"
#include <Eigen/Core>
#include <queue>

namespace fluid {
namespace algorithm {


class SineExtraction
{
  using ArrayXd = Eigen::ArrayXd;
  using VectorXd = Eigen::VectorXd;
  using ArrayXcd = Eigen::ArrayXcd;
  template <typename T>
  using vector = std::vector<T>;

public:
  SineExtraction(index maxFFTSize)
      : mBins(maxFFTSize / 2 + 1), mMaxFFTSize(maxFFTSize), mFFT(maxFFTSize)
  {}

  void init(index windowSize, index fftSize, index bandwidth)
  {
    mWindowSize = windowSize;
    mBins = fftSize / 2 + 1;
    mCurrentFrame = 0;
    mBuf = std::queue<ArrayXcd>();
    mScale = 1.0 / (windowSize / 4.0); // scale to original amplitude
    mBandwidth = bandwidth;
    computeWindowTransform();
    mTracking.init();
    mInitialized = true;
  }

  void computeWindowTransform()
  {
    index halfBW = mMaxFFTSize / 2;
    mWindowTransform = ArrayXd::Zero(mMaxFFTSize);
    ArrayXd window = ArrayXd::Zero(mWindowSize);
    WindowFuncs::map()[WindowFuncs::WindowTypes::kHann](mWindowSize, window);
    ArrayXcd transform =
        mFFT.process(Eigen::Map<ArrayXd>(window.data(), mWindowSize));
    for (index i = 0; i < halfBW; i++)
    {
      mWindowTransform(halfBW + i) = mWindowTransform(halfBW - i) =
          abs(transform(i));
    }
  }

  void processFrame(const ComplexVectorView in, ComplexMatrixView out,
                    double sampleRate)
  {
    assert(mInitialized);
    using Eigen::Array;
    using Eigen::ArrayXXcd;
    index fftSize = 2 * (mBins - 1);

    ArrayXcd frame = _impl::asEigen<Array>(in);
    mBuf.push(frame);
    ArrayXd mag = frame.abs().real();
    mag = mag * mScale;
    ArrayXd logMag = 20 * mag.max(epsilon).log10();

    vector<SinePeak> peaks;
    auto tmpPeaks = mPeakDetection.process(logMag, 0, -infinity, true, false);
    for (auto p : tmpPeaks)
    {
      if (p.second > mDeathThreshold)
      {
        double hz = sampleRate * p.first / fftSize;
        peaks.push_back({hz, p.second, false});
      }
    }
    double maxAmp = 20 * std::log10(mag.maxCoeff());
    mTracking.processFrame(peaks, maxAmp);
    vector<SinePeak> sinePeaks = mTracking.getActivePeaks();
    ArrayXd          frameSines = additiveSynthesis(sinePeaks, sampleRate);
    ArrayXXcd        result(mBins, 2);

    if (asSigned(mBuf.size()) <= mTracking.minTrackLength())
    {
      result.col(0) = ArrayXd::Zero(mBins);
      result.col(1) = ArrayXd::Zero(mBins);
    }
    else
    {
      ArrayXcd resultFrame = mBuf.front();
      ArrayXd  resultMag = resultFrame.abs().real();
      for (index i = 0; i < mBins; i++)
      {
        if (frameSines(i) >= resultMag(i))
        {
          result(i, 0) = resultFrame(i);
          result(i, 1) = 0;
        }
        else
        {
          double sineWeight = frameSines(i) / resultMag(i);
          result(i, 0) = resultFrame(i) * sineWeight;
          result(i, 1) = resultFrame(i) * (1 - sineWeight);
        }
      }
      mBuf.pop();
    }
    mTracking.prune();
    out = _impl::asFluid(result);
    mCurrentFrame++;
  }


  void reset() { mCurrentFrame = 0; }

  void setDeathThreshold(double threshold) { mDeathThreshold = threshold; }

  void setBirthLowThreshold(double threshold)
  {
    mTracking.setBirthLowThreshold(threshold);
  }

  void setBirthHighThreshold(double threshold)
  {
    mTracking.setBirthHighThreshold(threshold);
  }

  void setMinTrackLength(index minTrackLength)
  {
    if (minTrackLength != mTracking.getMinTrackLength())
    {
      mBuf = std::queue<ArrayXcd>();
      mTracking.setMinTrackLength(minTrackLength);
    }
  }

  void setMethod(index method) { mTracking.setMethod(method); }

  void setZetaA(double zetaA) { mTracking.setZetaA(zetaA); }

  void setZetaF(double zetaF) { mTracking.setZetaF(zetaF); }

  void setDelta(double delta) { mTracking.setDelta(delta); }

  bool initialized() { return mInitialized; }

private:
  ArrayXd additiveSynthesis(const vector<SinePeak> peaks, double sampleRate)
  {
    ArrayXd result = ArrayXd::Zero(mBins);
    for (auto& p : peaks) { result += synthesizePeak(p, sampleRate); }
    return result;
  }

  ArrayXd synthesizePeak(SinePeak p, double sampleRate)
  {
    index   halfBW = mBandwidth / 2;
    ArrayXd sine = ArrayXd::Zero(mBins);
    double  freqBin = p.freq * 2 * (mBins - 1) / sampleRate;
    if (freqBin >= mBins - 1) freqBin = mBins - 1;
    if (freqBin < 0) freqBin = 0;
    index  freqBinFloor = static_cast<index>(std::floor(freqBin));
    index  freqBinCeil =  static_cast<index>(std::ceil(freqBin));
    double amp = 0.5 * std::pow(10, p.logMag / 20);
    double incr =
        0.5 * static_cast<double>(mWindowTransform.size()) / (mBins - 1);
    double pos = mWindowTransform.size() / 2;
    for (index i = freqBinFloor; pos < mWindowTransform.size() - 2 &&
                                 i < std::min(freqBinCeil + halfBW, mBins - 1);
         i++, pos += incr)
    {
      index  floor = static_cast<index>(std::floor(pos));
      double frac1 = pos - floor;
      double val = frac1 * mWindowTransform(floor) +
                   (1 - frac1) * mWindowTransform(floor + 1);
      sine[i] = amp * val;
    }
    pos = mWindowTransform.size() / 2;
    for (index i = freqBinFloor;
         pos > 1 && i > std::max(freqBinFloor - halfBW, asSigned(0));
         i--, pos -= incr)
    {
      index  floor = static_cast<index>(std::floor(pos));
      double frac = pos - floor;
      double val = frac * mWindowTransform(floor) +
                   (1 - frac) * mWindowTransform(floor + 1);
      sine[i] = amp * val;
    }
    return sine;
  }

  PeakDetection        mPeakDetection;
  PartialTracking      mTracking;
  index                mBins{513};
  double               mDeathThreshold{-96.};
  index                mCurrentFrame{0};
  vector<SineTrack>    mTracks;
  std::queue<ArrayXcd> mBuf;
  ArrayXd              mWindowTransform;
  double               mScale{1.0};
  bool                 mInitialized{false};
  index                mBandwidth{76};
  index                mWindowSize{1024};
  index                mMaxFFTSize{16384};
  FFT                  mFFT;
};
} // namespace algorithm
} // namespace fluid
