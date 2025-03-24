#pragma once

// AAX includes
#include "AAX.h"
#include "wrapper.h"

#include AAX_ALIGN_FILE_BEGIN
#include AAX_ALIGN_FILE_ALG
#include AAX_ALIGN_FILE_END

struct SAAX_Wrapper_PrivateData
{
  ClapAsAAX* wrapper;
};

struct SAAX_Wrapper_AlgorithmicContext
{
  float** mAudioInputs;   ///< Audio input buffers
  float** mAudioOutputs;  ///< Audio output buffers, including any aux output stems.
  int32_t* mNumSamples;  ///< Number of samples in each buffer.  Bounded as per \ref AAE_EAudioBufferLengthNative.
                    // The exact value can vary from buffer to buffer.
  AAX_CTimestamp* mClock;  ///< Pointer to the global running time clock.

  AAX_IMIDINode* mInputNode;  ///< Buffered local MIDI input node. Used for incoming MIDI messages directed to the instrument.
  AAX_IMIDINode* mGlobalNode;  ///< Buffered global MIDI input node. Used for global events like beat updates in metronomes.
  AAX_IMIDINode* mTransportNode;  ///< Transport MIDI node.  Used for querying the state of the MIDI transport.
  //  AAX_IMIDINode*              mAdditionalInputMIDINodes[kMaxAdditionalMIDINodes];  ///< List of additional input MIDI nodes, if your plugin needs them.

  SAAX_Wrapper_PrivateData* mPrivateData;

  float** mMeters;  ///< Array of meter taps.  One meter value should be entered per tap for each render call.

  int64_t* mCurrentStateNum;  ///< State counter

  // perhaps we need more, later
};

#include AAX_ALIGN_FILE_BEGIN
#include AAX_ALIGN_FILE_RESET
#include AAX_ALIGN_FILE_END

void AAX_CALLBACK AAXWrapper_AlgorithmProcessFunction(
    SAAX_Wrapper_AlgorithmicContext* const inInstancesBegin[], const void* inInstancesEnd);

int32_t* AAX_CALLBACK AAXWrapper_inInstanceInitProc(const SAAX_Wrapper_AlgorithmicContext* inInstanceContextPtr, AAX_EComponentInstanceInitAction inAction);