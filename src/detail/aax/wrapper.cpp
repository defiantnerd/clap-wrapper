/*
    CLAP as AAX

    Copyright (c) 2024 Timo Kaluza (defiantnerd)

    This file is part of the clap-wrappers project which is released under MIT License.
    See file LICENSE or go to https://github.com/free-audio/clap-wrapper for full license details.
    
    This AAX opens a CLAP plugin and matches all corresponding AAX calls to it.
    For the AAX Host it is a AAX plugin, for the CLAP plugin it is a CLAP host.

*/

#include "wrapper.h"
#include "clap_proxy.h"
#include "AAX.h"
#include "AAX_ICollection.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IEffectDescriptor.h"
#include "AAX_IPropertyMap.h"
#include "AAX_Exception.h"
#include "AAX_Errors.h"
#include "AAX_Assert.h"
#include "AAX_Init.h"

AAX_Result GetEffectDescriptionsXX(AAX_ICollection* outCollection)
{
  AAX_CheckedResult err;
  AAX_IEffectDescriptor* const effectDescriptor = outCollection->NewDescriptor();
  if (effectDescriptor == NULL) err = AAX_ERROR_NULL_OBJECT;

  return err;
}
