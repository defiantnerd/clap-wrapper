#pragma once
/*
    CLAP as AAX

    Copyright (c) 2024 Timo Kaluza (defiantnerd)

    This file is part of the clap-wrappers project which is released under MIT License.
    See file LICENSE or go to https://github.com/free-audio/clap-wrapper for full license details.
    
    This AAX opens a CLAP plugin and matches all corresponding AAX calls to it.
    For the AAX Host it is a AAX plugin, for the CLAP plugin it is a CLAP host.

*/

#if WIN32
#pragma warning( disable : 5033 )   // \aax-sdk-2-8-1\Interfaces\AAX_Atomic.h(191,26): warning C5033: 'register' is no longer a supported storage class
#endif

// AAX headers
//
// Parent class
#include "AAX_CEffectParameters.h"
// #include "Topology/AAX_CMonolithicParameters.h"

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
#include "parameter.h"

// cpp stuff
#include <map>
#include <string>
#include <mutex>

class AAX_ICollection;
class Wrapped_AAX_GUI;

AAX_Result GetEffectDescriptions ( AAX_ICollection * outDescriptions );
AAX_CEffectParameters* AAX_CALLBACK ClapAsAAX_Create();

class ClapAsAAX : public AAX_CEffectParameters,
  public Clap::IHost,
  public Clap::IAutomation,
  public os::IPlugObject
{
public:
  friend class Wrapped_AAX_GUI;
  ClapAsAAX()
    : AAX_CEffectParameters()
   , Clap::IHost()
   , Clap::IAutomation()
   , os::IPlugObject()
   , _os_attached([this] { os::attach(this); }, [this] { os::detach(this); })
 {
 }

 AAX_Result EffectInit() override;
 AAX_Result ResetFieldData(AAX_CFieldIndex iFieldIndex, void* oData, uint32_t iDataSize) const override;
 AAX_Result TimerWakeup() override;
 AAX_Result GetParameterIsAutomatable(AAX_CParamID iParameterID, AAX_CBoolean* itIs)  const override;
 AAX_Result GetParameterNumberOfSteps(AAX_CParamID iParameterID, int32_t* aNumSteps)  const override;
 AAX_Result GetParameterValueString(AAX_CParamID iParameterID, AAX_IString* oValueString, int32_t iMaxLength)  const override;
 AAX_Result GetParameterValueFromString(AAX_CParamID iParameterID, double* oValuePtr, const AAX_IString& iValueString)  const override;
 AAX_Result GetParameterStringFromValue(AAX_CParamID iParameterID, double value, AAX_IString* valueString, int32_t maxLength)  const override;
 AAX_Result GetParameterName(AAX_CParamID iParameterID, AAX_IString* oName) const override;
 AAX_Result GetParameterNameOfLength(AAX_CParamID iParameterID, AAX_IString* oName,
                                     int32_t iNameLength) const override;

 //---The Clunky Chunk-------------------------------------------------------------------
 AAX_Result GetNumberOfChunks(int32_t* oNumChunks) const override;
 AAX_Result GetChunkIDFromIndex(int32_t iIndex, AAX_CTypeID* oChunkID) const override;
 AAX_Result GetChunkSize(AAX_CTypeID iChunkID, uint32_t* oSize) const override;
 AAX_Result GetChunk(AAX_CTypeID iChunkID, AAX_SPlugInChunk* oChunk) const override;
 AAX_Result SetChunk(AAX_CTypeID iChunkID, const AAX_SPlugInChunk* iChunk) override;

 //---Clap::IHost------------------------------------------------------------------------

  void setupWrapperSpecifics(const clap_plugin_t* plugin) override;

  void setupAudioBusses(const clap_plugin_t* plugin,
                        const clap_plugin_audio_ports_t* audioports) override;
  void setupMIDIBusses(const clap_plugin_t* plugin, const clap_plugin_note_ports_t* noteports) override;
  void setupParameters(const clap_plugin_t* plugin, const clap_plugin_params_t* params) override;

  void param_rescan(clap_param_rescan_flags flags) override;
  void param_clear(clap_id param, clap_param_clear_flags flags) override;
  void param_request_flush() override;

  bool gui_can_resize() override;
  bool gui_request_resize(uint32_t width, uint32_t height) override;
  bool gui_request_show() override;
  bool gui_request_hide() override;

  void latency_changed() override;

  void tail_changed() override;

  void mark_dirty() override;

  void restartPlugin() override;

  void request_callback() override;

  // clap_timer support
  bool register_timer(uint32_t period_ms, clap_id* timer_id) override;
  bool unregister_timer(clap_id timer_id) override;

  const char* host_get_name() override;

  bool supportsContextMenu() const override;
  // context_menu
  bool context_menu_populate(const clap_context_menu_target_t* target,
                             const clap_context_menu_builder_t* builder) override;
  bool context_menu_perform(const clap_context_menu_target_t* target, clap_id action_id) override;
  bool context_menu_can_popup() override;
  bool context_menu_popup(const clap_context_menu_target_t* target, int32_t screen_index, int32_t x,
                          int32_t y) override;

protected:
  void onIdle() override {};
  Clap::Library* _library = nullptr;
  std::shared_ptr<Clap::Plugin> _plugin;

  void* _creationcontext = nullptr;  // context from the CLAP library 
  os::State _os_attached;

  std::string _wrapper_hostname = "CLAP-As-AAX-Wrapper";
  
  std::map<std::string, std::unique_ptr<AAXWrappedParameterInfo_t>> _parameterMap;

  AAX_IController* _aax_ctrl = nullptr;

  mutable Clap::StateMemento _state;

private:
    // from Clap::IAutomation
  void onBeginEdit(clap_id id) override;
  void onPerformEdit(const clap_event_param_value_t* value) override;
  void onEndEdit(clap_id id) override;
};
