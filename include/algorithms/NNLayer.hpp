#pragma once

#include "NNFuncs.hpp"
#include "data/FluidIndex.hpp"
#include <Eigen/Core>

namespace fluid {
namespace algorithm {

class NNLayer {
  using MatrixXd = Eigen::MatrixXd;
  using VectorXd = Eigen::VectorXd;
  using Activation = NNActivations::Activation;
  using LayerData = std::tuple<RealMatrixView, RealVectorView, index>;

public:
  NNLayer(index inputSize, index outputSize, index actType) {
    using namespace Eigen;
    mWeights = MatrixXd::Ones(inputSize, outputSize);
    mBiases = VectorXd::Zero(outputSize);
    mActType = actType;
    mActivation = static_cast<Activation>(actType);
  }

  void init(Eigen::Ref<MatrixXd> weights, Eigen::Ref<VectorXd> biases, index actType) {
    mWeights = weights;
    mBiases = biases;
    mActType = actType;
    mActivation = static_cast<Activation>(actType);
    initGrads();
  }

  void init() {
    double dev = std::sqrt(6.0 / (mWeights.rows() + mWeights.cols()));
    mWeights = dev * MatrixXd::Random(mWeights.rows(), mWeights.cols()).array();
    mBiases = VectorXd::Zero(mWeights.cols());
    initGrads();
  }

  MatrixXd getWeights() const {return mWeights;}
  VectorXd getBiases() const {return mBiases;}
  index getActType() const {return mActType;}

  void initGrads(){
    mWeightsGrad = MatrixXd::Zero(mWeights.rows(), mWeights.cols());
    mBiasesGrad = VectorXd::Zero(mWeights.cols());
    mPrevWeightsUpdate = MatrixXd::Zero(mWeights.rows(), mWeights.cols());
    mPrevBiasesUpdate = VectorXd::Zero(mWeights.cols());
  }

  index inputSize() const { return mWeights.rows(); }

  index outputSize() const { return mWeights.cols(); }

  void forward(Eigen::Ref<MatrixXd> in, Eigen::Ref<MatrixXd> out) {
    mInput = in;
    MatrixXd WT = mWeights.transpose();
    MatrixXd IT = mInput.transpose();
    MatrixXd Z = ((WT * IT).colwise() + mBiases).transpose();
    mOutput = MatrixXd::Zero(out.rows(), out.cols());
    NNActivations::activation()[mActivation](Z, mOutput);
    out = mOutput;
  }

  void backward(Eigen::Ref<MatrixXd> outGrad,
                Eigen::Ref<MatrixXd> inGrad) { // going backwards, so out is in
    MatrixXd dAct = MatrixXd::Zero(mOutput.rows(), mOutput.cols());
    NNActivations::derivative()[mActivation](mOutput, dAct);
    MatrixXd actGrad = dAct.array() * outGrad.array();
    double norm = 1.0 / mInput.rows();
    mWeightsGrad = norm * (mInput.transpose() * actGrad);
    inGrad = actGrad * mWeights.transpose();
    mBiasesGrad = actGrad.colwise().mean();
  }

  void update(double learningRate, double momentum) {
    MatrixXd wUpdate =
        (momentum * mPrevWeightsUpdate) +
        ((1 - momentum) * learningRate * mWeightsGrad);
    VectorXd bUpdate =
            (momentum * mPrevBiasesUpdate) +
            ((1 - momentum) * learningRate * mBiasesGrad);
    mWeights = mWeights - wUpdate;
    mBiases = mBiases - bUpdate;
    mPrevWeightsUpdate = wUpdate;
    mPrevBiasesUpdate = bUpdate;
  }

private:
  MatrixXd mWeights;
  VectorXd mBiases;
  index mActType;
  Activation mActivation;

  MatrixXd mWeightsGrad;
  VectorXd mBiasesGrad;

  MatrixXd mPrevWeightsUpdate;
  VectorXd mPrevBiasesUpdate;

  MatrixXd mInput;
  MatrixXd mOutput;
};
} // namespace algorithm
} // namespace fluid
