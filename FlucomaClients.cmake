# Part of the Fluid Corpus Manipulation Project (http://www.flucoma.org/)
# Copyright University of Huddersfield.
# Licensed under the BSD-3 License.
# See license.md file in the project root for full license information.
# This project has received funding from the European Research Council (ERC)
# under the European Union’s Horizon 2020 research and innovation programme
# (grant agreement No 725899).

cmake_minimum_required (VERSION 3.11)

function(add_client) 
  # Define the supported set of keywords
  set(noValues NOINSTALL)
  set(singleValues CLASS)
  set(multiValues GROUP TAGS)
  # Process the arguments passed in
  include(CMakeParseArguments)
  cmake_parse_arguments(ARG
  "${noValues}"
  "${singleValues}"
  "${multiValues}"
  ${ARGN})  

list(LENGTH ARG_UNPARSED_ARGUMENTS NUM_PLAIN_ARGS)
  
if(NUM_PLAIN_ARGS LESS 2)
  message(FATAL_ERROR "add_client called without arguments for object name and header file")
endif() 

if(NOT ARG_CLASS)
  message(FATAL_ERROR "add_client: missing CLASS keyword argument")
endif() 

list(GET ARG_UNPARSED_ARGUMENTS 0 name)  
list(GET ARG_UNPARSED_ARGUMENTS 1 header)  

if(ARG_NOINSTALL)
  set(install true)
else()
  set(install false)
endif()

if(ARG_GROUP)
  set(group ${ARG_GROUP})
else ()
  set(group NONE)  
endif() 

if(ARG_TAGS)
  foreach(tag ${ARG_TAGS})
    set_property(GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_${tag} ON)
  endforeach()
endif() 

set_property(GLOBAL APPEND PROPERTY FLUID_CORE_CLIENTS ${name})
set_property(GLOBAL APPEND PROPERTY FLUID_CORE_CLIENTS_${group} ${name})
set_property(GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_HEADER ${header})
set_property(GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_CLASS ${ARG_CLASS})
set_property(GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_INSTALL ${install})    
endfunction() 

function (add_kr_in_client) 
  add_client(${ARGN} TAGS KR_IN)  
endfunction() 

function(get_client_group group var)
  get_property(clients GLOBAL PROPERTY FLUID_CORE_CLIENTS_${group})
  set(${var} ${clients} PARENT_SCOPE)
endfunction()

function(get_core_clients var)
  get_property(clients GLOBAL PROPERTY FLUID_CORE_CLIENTS)
  set(${var} ${clients} PARENT_SCOPE)
endfunction()

function(get_core_client_header name var)
  get_property(client_info GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_HEADER)
  set(${var} ${client_info} PARENT_SCOPE)
endfunction()

function(get_core_client_class name var)
  get_property(client_info GLOBAL PROPERTY FLUID_CORE_CLIENTS_${name}_CLASS)
  set(${var} ${client_info} PARENT_SCOPE)
endfunction()
# 
add_client(AmpFeature clients/rt/AmpFeatureClient.hpp CLASS RTAmpFeatureClient )
add_client(AmpGate clients/rt/AmpGateClient.hpp CLASS RTAmpGateClient )
add_client(AmpSlice clients/rt/AmpSliceClient.hpp CLASS RTAmpSliceClient )
add_client(AudioTransport clients/rt/AudioTransportClient.hpp CLASS RTAudioTransportClient )
add_client(BufAmpFeature clients/rt/AmpFeatureClient.hpp CLASS NRTThreadedAmpFeatureClient )
add_client(BufAmpGate clients/rt/AmpGateClient.hpp CLASS NRTThreadedAmpGateClient )
add_client(BufAmpSlice clients/rt/AmpSliceClient.hpp CLASS NRTThreadedAmpSliceClient )
add_client(BufAudioTransport clients/rt/AudioTransportClient.hpp CLASS NRTThreadedAudioTransportClient )
add_client(BufChroma clients/rt/ChromaClient.hpp CLASS NRTThreadedChromaClient )
add_client(BufCompose clients/nrt/BufComposeClient.hpp CLASS NRTThreadedBufComposeClient )
add_client(BufFlatten clients/nrt/BufFlattenClient.hpp CLASS NRTThreadedBufFlattenClient )
add_client(BufHPSS clients/rt/HPSSClient.hpp CLASS NRTThreadedHPSSClient )
add_client(BufLoudness clients/rt/LoudnessClient.hpp CLASS NRTThreadedLoudnessClient )
add_client(BufMFCC clients/rt/MFCCClient.hpp CLASS NRTThreadedMFCCClient )
add_client(BufMelBands clients/rt/MelBandsClient.hpp CLASS NRTThreadedMelBandsClient )
add_client(BufNMF clients/nrt/NMFClient.hpp CLASS NRTThreadedNMFClient )
add_client(BufNMFCross clients/nrt/NMFCrossClient.hpp CLASS NRTNMFCrossClient )
add_client(BufNMFSeed clients/nrt/NMFSeedClient.hpp CLASS NRTThreadedNMFSeedClient )
add_client(BufNoveltyFeature clients/rt/NoveltyFeatureClient.hpp CLASS NRTThreadedNoveltyFeatureClient )
add_client(BufNoveltySlice clients/rt/NoveltySliceClient.hpp CLASS NRTThreadingNoveltySliceClient )
add_client(BufOnsetFeature clients/rt/OnsetFeatureClient.hpp CLASS NRTThreadedOnsetFeatureClient )
add_client(BufOnsetSlice clients/rt/OnsetSliceClient.hpp CLASS NRTThreadingOnsetSliceClient )
add_client(BufPitch clients/rt/PitchClient.hpp CLASS NRTThreadedPitchClient )
add_client(BufSTFT clients/nrt/BufSTFTClient.hpp CLASS NRTThreadedBufferSTFTClient )
add_client(BufScale clients/nrt/BufScaleClient.hpp CLASS NRTThreadedBufferScaleClient )
add_client(BufSelect clients/nrt/BufSelectClient.hpp CLASS NRTThreadingSelectClient )
add_client(BufSelectEvery clients/nrt/BufSelectEveryClient.hpp CLASS NRTThreadingSelectEveryClient )
add_client(BufSineFeature clients/rt/SineFeatureClient.hpp CLASS NRTThreadedSineFeatureClient )
add_client(BufSines clients/rt/SinesClient.hpp CLASS NRTThreadedSinesClient )
add_client(BufSpectralShape clients/rt/SpectralShapeClient.hpp CLASS NRTThreadedSpectralShapeClient )
add_client(BufStats clients/nrt/BufStatsClient.hpp CLASS NRTThreadedBufferStatsClient )
add_client(BufThreadDemo clients/nrt/FluidThreadTestClient.hpp CLASS NRTThreadedThreadTestClient )
add_client(BufThresh clients/nrt/BufThreshClient.hpp CLASS NRTThreadedBufferThreshClient )
add_client(BufTransientSlice clients/rt/TransientSliceClient.hpp CLASS NRTThreadedTransientSliceClient )
add_client(BufTransients clients/rt/TransientClient.hpp CLASS NRTThreadedTransientsClient )
add_client(Chroma clients/rt/ChromaClient.hpp CLASS RTChromaClient )
add_client(Gain clients/rt/GainClient.hpp CLASS RTGainClient NOINSTALL)
add_client(HPSS clients/rt/HPSSClient.hpp CLASS RTHPSSClient )
add_client(Loudness clients/rt/LoudnessClient.hpp CLASS RTLoudnessClient )
add_client(MFCC clients/rt/MFCCClient.hpp CLASS RTMFCCClient )
add_client(MelBands clients/rt/MelBandsClient.hpp CLASS RTMelBandsClient )
add_client(NMFFilter clients/rt/NMFFilterClient.hpp CLASS RTNMFFilterClient )
add_client(NMFMatch clients/rt/NMFMatchClient.hpp CLASS RTNMFMatchClient )
add_client(NMFMorph clients/rt/NMFMorphClient.hpp CLASS RTNMFMorphClient )
add_client(NoveltyFeature clients/rt/NoveltyFeatureClient.hpp CLASS RTNoveltyFeatureClient )
add_client(NoveltySlice clients/rt/NoveltySliceClient.hpp CLASS RTNoveltySliceClient )
add_client(OnsetFeature clients/rt/OnsetFeatureClient.hpp CLASS RTOnsetFeatureClient )
add_client(OnsetSlice clients/rt/OnsetSliceClient.hpp CLASS RTOnsetSliceClient )
add_client(Pitch clients/rt/PitchClient.hpp CLASS RTPitchClient )
add_client(STFTPass clients/rt/BaseSTFTClient.hpp CLASS RTSTFTPassClient NOINSTALL)
add_client(SineFeature clients/rt/SineFeatureClient.hpp CLASS RTSineFeatureClient )
add_client(Sines clients/rt/SinesClient.hpp CLASS RTSinesClient )
add_client(SpectralShape clients/rt/SpectralShapeClient.hpp CLASS RTSpectralShapeClient )
add_kr_in_client(Stats clients/rt/RunningStatsClient.hpp CLASS RunningStatsClient )
add_client(TransientSlice clients/rt/TransientSliceClient.hpp CLASS RTTransientSliceClient )
add_client(Transients clients/rt/TransientClient.hpp CLASS RTTransientClient )

#lib manipulation client group 
add_client(DataSet clients/nrt/DataSetClient.hpp CLASS NRTThreadedDataSetClient GROUP MANIPULATION)
add_client(DataSeries clients/nrt/DataSeriesClient.hpp CLASS NRTThreadedDataSeriesClient GROUP MANIPULATION)
add_client(DataSetQuery clients/nrt/DataSetQueryClient.hpp CLASS NRTThreadedDataSetQueryClient GROUP MANIPULATION)
add_client(LabelSet clients/nrt/LabelSetClient.hpp CLASS NRTThreadedLabelSetClient GROUP MANIPULATION)
add_client(KDTree clients/nrt/KDTreeClient.hpp CLASS NRTThreadedKDTreeClient GROUP MANIPULATION)
add_client(KMeans clients/nrt/KMeansClient.hpp CLASS NRTThreadedKMeansClient GROUP MANIPULATION)
add_client(SKMeans clients/nrt/SKMeansClient.hpp CLASS NRTThreadedSKMeansClient GROUP MANIPULATION)
add_client(KNNClassifier clients/nrt/KNNClassifierClient.hpp CLASS NRTThreadedKNNClassifierClient GROUP MANIPULATION)
add_client(KNNRegressor clients/nrt/KNNRegressorClient.hpp CLASS NRTThreadedKNNRegressorClient GROUP MANIPULATION)
add_client(Normalize clients/nrt/NormalizeClient.hpp CLASS NRTThreadedNormalizeClient GROUP MANIPULATION)
add_client(RobustScale clients/nrt/RobustScaleClient.hpp CLASS NRTThreadedRobustScaleClient GROUP MANIPULATION)
add_client(Standardize clients/nrt/StandardizeClient.hpp CLASS NRTThreadedStandardizeClient GROUP MANIPULATION)
add_client(PCA clients/nrt/PCAClient.hpp CLASS NRTThreadedPCAClient GROUP MANIPULATION)
add_client(MDS clients/nrt/MDSClient.hpp CLASS NRTThreadedMDSClient GROUP MANIPULATION)
add_client(UMAP clients/nrt/UMAPClient.hpp CLASS NRTThreadedUMAPClient GROUP MANIPULATION)
add_client(MLPRegressor clients/nrt/MLPRegressorClient.hpp CLASS NRTThreadedMLPRegressorClient GROUP MANIPULATION)
add_client(MLPClassifier clients/nrt/MLPClassifierClient.hpp CLASS NRTThreadedMLPClassifierClient GROUP MANIPULATION)
add_client(Grid clients/nrt/GridClient.hpp CLASS NRTThreadedGridClient GROUP MANIPULATION)
add_client(LSTMClassifier clients/nrt/LSTMClassifierClient.hpp CLASS NRTThreadedLSTMClassifierClient GROUP MANIPULATION)
add_client(LSTMRegressor clients/nrt/LSTMRegressorClient.hpp CLASS NRTThreadedLSTMRegressorClient GROUP MANIPULATION)
add_client(LSTMForecaster clients/nrt/LSTMForecasterClient.hpp CLASS NRTThreadedLSTMForecasterClient GROUP MANIPULATION)