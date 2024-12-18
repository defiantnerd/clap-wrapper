/*
    CLAP as AAX

    Copyright (c) 2024 Timo Kaluza (defiantnerd)

    This file is part of the clap-wrappers project which is released under MIT License.
    See file LICENSE or go to https://github.com/free-audio/clap-wrapper for full license details.
    
    This AAX opens a CLAP plugin and matches all corresponding AAX calls to it.
    For the AAX Host it is a AAX plugin, for the CLAP plugin it is a CLAP host.

*/

// AAX headers
//
// Parent class
#include "AAX_CEffectParameters.h"
//
// Describe
#include "AAX_IEffectDescriptor.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IPropertyMap.h"
//
// Utilities
#include "AAX_CAtomicQueue.h"
#include "AAX_IParameter.h"
#include "AAX_IMIDINode.h"
#include "AAX_IString.h"

// Wrapper
#include "detail/os/osutil.h"
#include "detail/clap/automation.h"
#include "clap_proxy.h"

#include <mutex>


class ClapAsAAX : public AAX_CEffectParameters,
  public Clap::IHost,
  public Clap::IAutomation,
  public os::IPlugObject
{
public:
  ClapAsAAX()
    : AAX_CEffectParameters()
    , Clap::IHost()
    , Clap::IAutomation()
    , os::IPlugObject()
    , _os_attached([this] { os::attach(this); }, [this] { os::detach(this); })
  {}
protected:
  void onIdle() override {};
  Clap::Library* _library = nullptr;
  int _libraryIndex = 0;
  std::shared_ptr<Clap::Plugin> _plugin;

  void* _creationcontext = nullptr;  // context from the CLAP library 
  os::State _os_attached;
};
