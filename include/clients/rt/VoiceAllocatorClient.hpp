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

#include "../common/AudioClient.hpp"
#include "../common/BufferedProcess.hpp"
#include "../common/FluidBaseClient.hpp"
#include "../common/FluidNRTClientWrapper.hpp"
#include "../common/ParameterConstraints.hpp"
#include "../common/ParameterSet.hpp"
#include "../common/ParameterTypes.hpp"
#include "../../data/TensorTypes.hpp"
#include "../../algorithms/util/PartialTracking.hpp"

namespace fluid {
namespace client {
namespace voiceallocator {

template <typename T>
using HostVector = FluidTensorView<T, 1>;

enum VoiceAllocatorParamIndex {
  kNVoices,
  kPrioritisedVoices,
  kBirthLowThreshold,
  kBirthHighTreshold,
  kMinTrackLen,
  kTrackMagRange,
  kTrackFreqRange,
  kTrackProb
};

constexpr auto VoiceAllocatorParams = defineParameters(
    LongParamRuntimeMax<Primary>( "numVoices", "Number of Voices", 1, Min(1)),
    EnumParam("prioritisedVoices", "Prioritised Voice Quality", 0, "Lowest Frequency", "Loudest Magnitude"),
    FloatParam("birthLowThreshold", "Track Birth Low Frequency Threshold", -24, Min(-144), Max(0)),
    FloatParam("birthHighThreshold", "Track Birth High Frequency Threshold", -60, Min(-144), Max(0)),
    LongParam("minTrackLen", "Minimum Track Length", 1, Min(1)),
    FloatParam("trackMagRange", "Tracking Magnitude Range (dB)", 15., Min(1.), Max(200.)),
    FloatParam("trackFreqRange", "Tracking Frequency Range (Hz)", 50., Min(1.), Max(10000.)),
    FloatParam("trackProb", "Tracking Matching Probability", 0.5, Min(0.0), Max(1.0))
    );

class VoiceAllocatorClient : public FluidBaseClient,
                             public ControlIn,
                             ControlOut
{
    template <typename T>
    using vector = rt::vector<T>;
    using VoicePeak = algorithm::VoicePeak;
    using SinePeak = algorithm::SinePeak;

public:
  using ParamDescType = decltype(VoiceAllocatorParams);

  using ParamSetViewType = ParameterSetView<ParamDescType>;
  std::reference_wrapper<ParamSetViewType> mParams;

  void setParams(ParamSetViewType& p) { mParams = p; }

  template <size_t N>
  auto& get() const
  {
    return mParams.get().template get<N>();
  }

  static constexpr auto& getParameterDescriptors()
  {
    return VoiceAllocatorParams;
  }

  VoiceAllocatorClient(ParamSetViewType& p, FluidContext& c)
      : mParams(p), mTracking(c.allocator()),
      mSizeTracker{ 0 },
      mFreeVoices(c.allocator()), mActiveVoices(c.allocator()),
      mActiveVoiceData(0, c.allocator()),
      mFreqs(get<kNVoices>().max(), c.allocator()),
      mLogMags(get<kNVoices>().max(), c.allocator()),
      mVoiceIDs(get<kNVoices>().max(), c.allocator())
  {
    controlChannelsIn(2);
    controlChannelsOut({3, get<kNVoices>(), get<kNVoices>().max()});
    setInputLabels({"frequencies", "magnitudes"});
    setOutputLabels({"frequencies", "magnitudes", "state"});
    mTracking.init();
  }

  void init(index nVoices)
  {
      while (!mActiveVoices.empty()) { mActiveVoices.pop_back(); }
      while (!mFreeVoices.empty()) { mFreeVoices.pop(); }
      for (index i = 0; i < nVoices; ++i) { mFreeVoices.push(i); }
      mActiveVoiceData.resize(nVoices);
      for (VoicePeak each : mActiveVoiceData) { each = { 0, 0, 0 }; }
  }
    
    void reset(FluidContext& c) {
        init(get<kNVoices>());
    }

  template <typename T>
  void process(std::vector<HostVector<T>>& input,
               std::vector<HostVector<T>>& output, FluidContext& c)
  {
    index nVoices = get<kNVoices>();

    if (mSizeTracker.changed(nVoices))
    {
      controlChannelsOut({4, nVoices}); //update the dynamic out size
      init(nVoices);
    }

    vector<SinePeak> incomingVoices(0, c.allocator());
    for (index i = 0; i < input[0].size(); ++i)
    {
        if (input[1].row(i) != 0 && input[0].row(i) != 0)
        {
            double logMag = 20 * log10(std::max(static_cast<double>(input[1].row(i)), algorithm::epsilon));
            incomingVoices.push_back({ input[0].row(i), logMag, false });
        }
    }

    double maxAmp = -144;
    for (SinePeak voice : incomingVoices)
    {
        if (voice.logMag > maxAmp) { maxAmp = voice.logMag; }
    }

    mTracking.processFrame(incomingVoices, maxAmp, get<kMinTrackLen>(), get<kBirthLowThreshold>(), get<kBirthHighTreshold>(), 0, get<kTrackMagRange>(), get<kTrackFreqRange>(), get<kTrackProb>(), c.allocator());

    vector<VoicePeak> outgoingVoices(0, c.allocator());
    outgoingVoices = mTracking.getActiveVoices(c.allocator());
    outgoingVoices = sortVoices(outgoingVoices, get<kPrioritisedVoices>());
    if (outgoingVoices.size() > nVoices)
        outgoingVoices.resize(nVoices);
    outgoingVoices = allocatorAlgorithm(outgoingVoices, c.allocator());

    for (index i = 0; i < nVoices; ++i)
    {
        output[2].row(i) = static_cast<index>(outgoingVoices[i].state);
        output[1].row(i) = outgoingVoices[i].logMag;
        output[0].row(i) = outgoingVoices[i].freq;
    }

    mTracking.prune();
  }

  vector<VoicePeak> sortVoices(vector<VoicePeak>& incomingVoices, index sortingMethod)
  {
      switch (sortingMethod)
      {
      case 0: //lowest
          std::sort(incomingVoices.begin(), incomingVoices.end(),
                    [](const VoicePeak& voice1, const VoicePeak& voice2)
                    { return voice1.freq < voice2.freq; });
          break;
      case 1: //loudest
          std::sort(incomingVoices.begin(), incomingVoices.end(),
                    [](const VoicePeak& voice1, const VoicePeak& voice2)
                    { return voice1.logMag > voice2.logMag; });
          break;
      }
      return incomingVoices;
  }

  vector<VoicePeak> allocatorAlgorithm(vector<VoicePeak>& incomingVoices, Allocator& alloc)
  {
      //move released to free
      for (index existing = 0; existing < mActiveVoiceData.size(); ++existing)
      {
          if (mActiveVoiceData[existing].state == algorithm::VoiceState::kReleaseState)
              mActiveVoiceData[existing].state = algorithm::VoiceState::kFreeState;
      }

      //handle existing voices - killing or sustaining
      for (index existing = 0; existing < mActiveVoices.size(); ++existing)
      {
          bool killVoice = true;
          for (index incoming = 0; incoming < incomingVoices.size(); ++incoming)
          {
              //remove incoming voice events & allows corresponding voice to live if it already exists
              if (mActiveVoiceData[mActiveVoices[existing]].voiceID == incomingVoices[incoming].voiceID)
              {
                  killVoice = false;
                  mActiveVoiceData[mActiveVoices[existing]] = incomingVoices[incoming]; //update freq/mag
                  mActiveVoiceData[mActiveVoices[existing]].state = algorithm::VoiceState::kSustainState;
                  incomingVoices.erase(incomingVoices.begin() + incoming);
                  break;
              }
          }
          if (killVoice) //voice off
          {
              mActiveVoiceData[mActiveVoices[existing]].state = algorithm::VoiceState::kReleaseState;
              mFreeVoices.push(mActiveVoices[existing]);
              mActiveVoices.erase(mActiveVoices.begin() + existing);
              --existing;
          }
      }

      //handle new voice allocation
      for (index incoming = 0; incoming < incomingVoices.size(); ++incoming)
      {
          if (!mFreeVoices.empty()) //voice on
          {
              index newVoiceIndex = mFreeVoices.front();
              mFreeVoices.pop();
              mActiveVoices.push_back(newVoiceIndex);
              algorithm::VoiceState prevState = mActiveVoiceData[newVoiceIndex].state;
              mActiveVoiceData[newVoiceIndex] = incomingVoices[incoming];
              if (prevState == algorithm::VoiceState::kReleaseState) //mark as stolen
                  mActiveVoiceData[newVoiceIndex].state = algorithm::VoiceState::kStolenState;
          }
      }

      return mActiveVoiceData;
  }

  MessageResult<void> clear()
  {
    init(get<kNVoices>());
    return {};
  }

  static auto getMessageDescriptors()
  {
    return defineMessages(makeMessage("clear", &VoiceAllocatorClient::clear));
  }

  index latency() const { return 0; }

private:
    rt::queue<index>                           mFreeVoices;
    rt::deque<index>                           mActiveVoices;
    vector<VoicePeak>                           mActiveVoiceData;
    algorithm::PartialTracking                  mTracking;
    ParameterTrackChanges<index>                mSizeTracker;
    FluidTensor<double, 1>                      mFreqs;
    FluidTensor<double, 1>                      mLogMags;
    FluidTensor<double, 1>                      mVoiceIDs;
};

} // namespace voiceallocator

using VoiceAllocatorClient =
    ClientWrapper<voiceallocator::VoiceAllocatorClient>;

auto constexpr NRTVoiceAllocatorParams =
    makeNRTParams<voiceallocator::VoiceAllocatorClient>(
        InputBufferParam("freqIn", "Peak Frequencies Buffer"),
        InputBufferParam("magIn", "Peak Magnitudes Buffer"),
        BufferParam("freqOut", "Voice Frequencies Buffer"),
        BufferParam("magOut", "Voice Magnitudes Buffer"),
        BufferParam("voiceState", "Voice State"));

using NRTVoiceAllocator = NRTStreamAdaptor<voiceallocator::VoiceAllocatorClient,
                                           decltype(NRTVoiceAllocatorParams),
                                           NRTVoiceAllocatorParams, 2, 3>;

using NRTThreadedVoiceAllocator = NRTThreadingAdaptor<NRTVoiceAllocator>;

} // namespace client
} // namespace fluid
