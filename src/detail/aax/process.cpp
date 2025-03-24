#include "process.h"


void AAX_CALLBACK AAXWrapper_AlgorithmProcessFunction(
  SAAX_Wrapper_AlgorithmicContext* const inInstancesBegin[], const void* inInstancesEnd)
{
  // processing instances
  SAAX_Wrapper_AlgorithmicContext* AAX_RESTRICT instance = inInstancesBegin[0];
  for (SAAX_Wrapper_AlgorithmicContext* const* walk = inInstancesBegin; walk < inInstancesEnd; ++walk)
  {
    instance = *walk;

    // todo: process

  }
}

int32_t* AAX_CALLBACK AAXWrapper_inInstanceInitProc(const SAAX_Wrapper_AlgorithmicContext* inInstanceContextPtr, AAX_EComponentInstanceInitAction inAction)
{
  return 0;
}