/*
    CLAP as AAX

    Copyright (c) 2024 Timo Kaluza (defiantnerd)

    This file is part of the clap-wrappers project which is released under MIT License.
    See file LICENSE or go to https://github.com/free-audio/clap-wrapper for full license details.
    
    This AAX opens a CLAP plugin and matches all corresponding AAX calls to it.
    For the AAX Host it is a AAX plugin, for the CLAP plugin it is a CLAP host.

*/

#include "wrapper.h"
// #include "clap_proxy.h"
#include "AAX.h"
#include "AAX_ICollection.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IEffectDescriptor.h"
#include "AAX_IPropertyMap.h"
#include "AAX_Exception.h"
#include "AAX_Errors.h"
#include "AAX_Assert.h"
#include "AAX_Init.h"

// ----[CLAP]-----------------------------------------------------------------------

#include "factory.h"

// ---------------------------------------------------------------------------------
#include "detail/shared/util.h"

// ----[AAX WRAPPER]----------------------------------------------------------------
#include "process.h"
#include "util.h"
#include "clapwrapper/aax.h"
#include "plugview.h"

// ---------------------------------------------------------------------------------
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

uint32_t clapCategoriesToAAX(const char* const* clap_categories);  // categories.cpp

static void DescribeAlgorithmComponent(AAX_IComponentDescriptor* outDesc,
                                       const clap_plugin_descriptor_t* clapDescriptor)
{
  AAX_CheckedResult err;

  // Describe algorithm's context structure
  //
  // Add outputs, meters, info, etc
  err = outDesc->AddAudioIn(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mAudioInputs));
  err = outDesc->AddAudioOut(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mAudioOutputs));
  err = outDesc->AddAudioBufferLength(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mNumSamples));
  err = outDesc->AddClock(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mClock));

  // err = outDesc->AddMeters( AAX_FIELD_INDEX (SAAX_Wrapper_AlgorithmicContext, mMeters), setupInfo.mMeterIDs, static_cast<uint32_t>(setupInfo.mNumMeters) );
  err = outDesc->AddPrivateData(
      AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mMeters), sizeof(float),
      AAX_ePrivateDataOptions_DefaultOptions);  //Just here to fill the port.  Not used.

  // Register MIDI nodes. To avoid context corruption, register small blocks of private data for fields where a node is not needed
  AAX_CFieldIndex globalNodeID = AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mGlobalNode);
  AAX_CFieldIndex localInputNodeID = AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mInputNode);
  AAX_CFieldIndex transportNodeID = AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mTransportNode);

  if (true)  // setupInfo.mNeedsGlobalMIDI)
    err = outDesc->AddMIDINode(globalNodeID, AAX_eMIDINodeType_Global,
                               "MIDI Global" /*setupInfo.mGlobalMIDINodeName */,
                               1 /*setupInfo.mGlobalMIDIEventMask*/);
  else
    err = outDesc->AddPrivateData(
        globalNodeID, sizeof(float),
        AAX_ePrivateDataOptions_DefaultOptions);  //Just here to fill the port.  Not used.

  if (true)  // setupInfo.mNeedsInputMIDI)
    err = outDesc->AddMIDINode(localInputNodeID, AAX_eMIDINodeType_LocalInput,
                               "MIDI IN" /*setupInfo.mInputMIDINodeName*/,
                               1 /*setupInfo.mInputMIDIChannelMask*/);
  else
    err = outDesc->AddPrivateData(
        localInputNodeID, sizeof(float),
        AAX_ePrivateDataOptions_DefaultOptions);  //Just here to fill the port.  Not used.

  if (true)  // setupInfo.mNeedsTransport)
    err = outDesc->AddMIDINode(transportNodeID, AAX_eMIDINodeType_Transport, "Transport", 0xffff);
  else
    err = outDesc->AddPrivateData(
        transportNodeID, sizeof(float),
        AAX_ePrivateDataOptions_DefaultOptions);  //Just here to fill the port.  Not used.

  //Add pointer to the data model instance and other interesting information.
  err =
      outDesc->AddPrivateData(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mPrivateData),
                              sizeof(SAAX_Wrapper_PrivateData), AAX_ePrivateDataOptions_DefaultOptions);

  //Add a "state number" counter for deferred parameter updates
  err = outDesc->AddDataInPort(AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mCurrentStateNum),
                               sizeof(uint64_t));
#if 0
	err = outDesc->AddAudioIn (eAlgFieldID_AudioIn);
	err = outDesc->AddAudioOut (eAlgFieldID_AudioOut);
	err = outDesc->AddAudioBufferLength (eAlgFieldID_BufferSize);
	static_assert(eMeterTap_Count == sizeof(cDemoGain_MeterID)/sizeof(AAX_CTypeID), "unexpected meter tap array size");
	err = outDesc->AddMeters ( eAlgFieldID_Meters, cDemoGain_MeterID, eMeterTap_Count );
    //
	// Register context fields as communications destinations (i.e. input)
	err = outDesc->AddDataInPort ( eAlgPortID_BypassIn, sizeof (int32_t) );
	err = outDesc->AddDataInPort ( eAlgPortID_CoefsGainIn, sizeof (SDemoGain_CoefsGain) );

#endif

  // Register processing callbacks
  //
  // Create a property map
  AAX_IPropertyMap* const properties = outDesc->NewPropertyMap();
  if (!properties) err = AAX_ERROR_NULL_OBJECT;
  //
  // Generic properties

  err = properties->AddProperty(AAX_eProperty_ManufacturerID, AAXIDfromString(clapDescriptor->vendor));
  err = properties->AddProperty(AAX_eProperty_ProductID, AAXIDfromString(clapDescriptor->id));
  err = properties->AddProperty(AAX_eProperty_CanBypass, true);
  // err = properties->AddProperty(AAX_eProperty_UsesClientGUI, true);  // Uses auto-GUI by the host

  err = properties->AddProperty(AAX_eProperty_RequiresChunkCallsOnMainThread,
                                true);  // for the CLAP this is mandatory
  err = properties->AddProperty(AAX_eProperty_Constraint_Topology,
                                AAX_eConstraintTopology_Monolithic);  // no separate UI and DSP
  //
  //
  // Stem format -specific properties
  err = properties->AddProperty(AAX_eProperty_InputStemFormat, AAX_eStemFormat_Stereo);
  err = properties->AddProperty(AAX_eProperty_OutputStemFormat, AAX_eStemFormat_Stereo);

  // multi/mono should not be the same
  err = properties->AddProperty(AAX_eProperty_Constraint_MultiMonoSupport, 0);
  //
  // ID properties
  std::string p(clapDescriptor->id);
  // TODO: enumerate bus combinations

  p.append("-stereo");

  err = properties->AddProperty(AAX_eProperty_PlugInID_Native,
                                AAXIDfromString(p.c_str()));  // cDemoGain_PlugInID_Native
  err =
      properties->AddProperty(AAX_eProperty_Constraint_Location, AAX_eConstraintLocationMask_DataModel);

  //	err = properties->AddProperty ( AAX_eProperty_PlugInID_AudioSuite, cDemoGain_PlugInID_AudioSuite );	// for offline processing
  // 	err = properties->AddProperty ( AAX_eProperty_PlugInID_TI, cDemoGain_PlugInID_TI );

  // Register Native callback
  err = outDesc->AddProcessProc_Native(AAXWrapper_AlgorithmProcessFunction, properties);

#if 0
	no TI in clap
	// TI-specific properties
#ifndef AAX_TI_BINARY_IN_DEVELOPMENT  // Define this macro when using a debug TI DLL to allocate only 1 instance per chip
	err = properties->AddProperty ( AAX_eProperty_TI_InstanceCycleCount, 102 );
	err = properties->AddProperty ( AAX_eProperty_TI_SharedCycleCount, 70 );
#endif
	err = properties->AddProperty ( AAX_eProperty_DSP_AudioBufferLength, AAX_eAudioBufferLengthDSP_Default );
	
	// Register TI callback
	err = outDesc->AddProcessProc_TI ("DemoGain_MM_TI_Example.dll", "AlgEntry", properties );
#endif
}

static AAX_Result DescribeEffectFromClap(AAX_IEffectDescriptor* outDescriptor,
                                         const clap_plugin_descriptor_t* clapDescriptor)
{
  using namespace CLAPAAX;

  // MessageBoxA(NULL, "Debugger", "Halted!", MB_OK);

  AAX_CheckedResult err;
  AAX_IComponentDescriptor* const compDesc = outDescriptor->NewComponentDescriptor();
  if (!compDesc) err = AAX_ERROR_NULL_OBJECT;

  // add the plugin name(s)
  os::log("generating names:");
  auto list = generateShortStrings(clapDescriptor->name);
  for (const auto& e : list)
  {
    os::log(e.c_str());
    err = outDescriptor->AddName(e.c_str());
  }

  err = outDescriptor->AddCategory(clapCategoriesToAAX(clapDescriptor->features));

  // Effect components
  //
  //
  //
  // Algorithm component
  {
    // repeat for each bus config
    err = compDesc->Clear();
    DescribeAlgorithmComponent(compDesc, clapDescriptor);
    err = outDescriptor->AddComponent(compDesc);
  }
  // plugin
  err = outDescriptor->AddProcPtr(reinterpret_cast<void*>(ClapAsAAX_Create),
                                  kAAX_ProcPtrID_Create_EffectParameters);

  // GUI
  err = outDescriptor->AddProcPtr((void*)Wrapped_AAX_GUI_Create, kAAX_ProcPtrID_Create_EffectGUI);

#if 0
	// Data model
	err = outDescriptor->AddResourceInfo ( AAX_eResourceType_PageTable, "DemoGainPages.xml" );
	
	// Effect's meter display properties
	//
	// Input meter
	{
		AAX_IPropertyMap* const meterProperties = outDescriptor->NewPropertyMap();
		if ( !meterProperties )
			err = AAX_ERROR_NULL_OBJECT;
		
		err = meterProperties->AddProperty ( AAX_eProperty_Meter_Type, AAX_eMeterType_Input );
		err = meterProperties->AddProperty ( AAX_eProperty_Meter_Orientation, AAX_eMeterOrientation_Default );
		err = outDescriptor->AddMeterDescription( cDemoGain_MeterID[eMeterTap_PreGain], "Input", meterProperties );
	}
	// Output meter
	{
		AAX_IPropertyMap* const meterProperties = outDescriptor->NewPropertyMap();
		if ( !meterProperties )
			err = AAX_ERROR_NULL_OBJECT;
		
		err = meterProperties->AddProperty ( AAX_eProperty_Meter_Type, AAX_eMeterType_Output );
		err = meterProperties->AddProperty ( AAX_eProperty_Meter_Orientation, AAX_eMeterOrientation_Default );
		err = outDescriptor->AddMeterDescription( cDemoGain_MeterID[eMeterTap_PostGain], "Output", meterProperties );
	}
#endif
  return err;
}
namespace cfg
{
clap_audio_port_configuration_request mono_out[]{{false, 0, 1, CLAP_PORT_MONO, nullptr}};

const clap_audio_port_configuration_request mono_in_out[]{
    {true, 0, 1, CLAP_PORT_MONO, nullptr},
    {false, 0, 1, CLAP_PORT_MONO, nullptr},
};

const clap_audio_port_configuration_request stereo_out[]{
    {false, 0, 2, CLAP_PORT_STEREO, nullptr},
};

const clap_audio_port_configuration_request stereo_in_out[]{
    {true, 0, 2, CLAP_PORT_STEREO, nullptr},
    {false, 0, 2, CLAP_PORT_STEREO, nullptr},
    {false, 1, 2, CLAP_PORT_STEREO, nullptr},
};
}  // namespace cfg

AAX_Result GetEffectDescriptions(AAX_ICollection* outCollection)
{
  AAX_CheckedResult err;

  // get CLAP factory and plugins
  auto* factory = CLAPAAX::guarantee_clap();

  if (factory->plugins.empty())
  {
    return AAX_ERROR_NULL_OBJECT;
  }
  // MessageBox(NULL, "ATTACH", "ME", MB_OK);
  // describe the plugins

  if (!factory->plugins.empty())
  {
    for (const auto i : factory->plugins)
    {
      AAX_IEffectDescriptor* const effectDescriptor = outCollection->NewDescriptor();

#if 1
      // why here? because we have factory and the clap-id
      static const clap_host_params_t micro_params = {
          [](const clap_host_t* host, clap_param_rescan_flags flags) -> void {},
          [](const clap_host_t* host, clap_id param_id, clap_param_clear_flags flags) -> void {},
          [](const clap_host_t* host) -> void {}};

      clap_host_t microhost = {
          CLAP_VERSION,
          nullptr,
          "aax_scanner",
          "clap",
          "",
          "1.0",
          [](const struct clap_host* host, const char* extension_id) -> const void*
          {
            if (extension_id) os::log(extension_id);
            if (!strcmp(CLAP_EXT_PARAMS, extension_id)) return &micro_params;
            return nullptr;
          },
          [](const struct clap_host* host) -> void {},  // request_restart
          [](const struct clap_host* host) -> void {},  // request_process
          [](const struct clap_host* host) -> void {},  // request_callback
      };
      try
      {
        auto* tmpplug =
            factory->_pluginFactory->create_plugin(factory->_pluginFactory, &microhost, i->id);
        tmpplug->init(tmpplug);
        auto k = (clap_plugin_configurable_audio_ports_t*)(tmpplug->get_extension(
            tmpplug, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS));
        if (k)
        {
          k->can_apply_configuration(tmpplug, cfg::stereo_in_out, 2);
        }
        tmpplug->destroy(tmpplug);
      }
      catch (std::exception& e)
      {
        os::log(e.what());
      }
#endif
      if (effectDescriptor)
      {
        AAX_SWALLOW_MULT(err = DescribeEffectFromClap(effectDescriptor, i);

                         // using the clap-plugin id to get it back from the host controller
                         err = outCollection->AddEffect(i->id, effectDescriptor););
      }
    }

    // use the first plugin name as package name
    auto& plug = factory->plugins[0];
    outCollection->SetManufacturerName(plug->vendor);
    outCollection->AddPackageName(plug->name);
    outCollection->SetPackageVersion(1);
  }
  else
  {
    err = AAX_ERROR_NULL_OBJECT;
  }

  return err;
}

AAX_CEffectParameters* AAX_CALLBACK ClapAsAAX_Create()
{
  // returning an empty shell
  os::log("-------------------------------------------------------------------------------------");
  return new ClapAsAAX();
}

AAX_Result ClapAsAAX::EffectInit()
{
  using namespace Clap;

  // when this is being called, the plugin is not connected at all, so
  // the actual (AAX) plugin id is being retrieved
  _aax_ctrl = Controller();
  AAX_CString m;
  _aax_ctrl->GetEffectID(&m);

  os::log("AAX Effect Init for ");
  os::log(m.StdString().c_str());

  _library = CLAPAAX::guarantee_clap();
  _plugin = Clap::Plugin::createInstance(_library->_pluginFactory, m.StdString(), this);

  if (_plugin)
  {
    if (_plugin->initialize())
    {
      // TODO: initialize calls wrapper specifics and sets up all busses etc.
      AAX_EStemFormat p;
      _aax_ctrl->GetInputStemFormat(&p);
      // AAX_eStemFormat_Mono
      // AAX_eStemFormat_Stereo
      if (p == AAX_eStemFormat_Mono)
      {
      }
      // TODO: set samplerate and initialize bus format

      // set signallatency
      _aax_ctrl->SetSignalLatency(100);
    }
  }
  return AAX_SUCCESS;
}

// this is called for each registered field, this is being used to reset the pointer
// to the actual wrapper plugin instance.
AAX_Result ClapAsAAX::ResetFieldData(AAX_CFieldIndex iFieldIndex, void* oData, uint32_t iDataSize) const
{
  //If this is the MonolithicParameters field, let's initialize it to our this pointer.
  if (iFieldIndex == AAX_FIELD_INDEX(SAAX_Wrapper_AlgorithmicContext, mPrivateData))
  {
    os::log("Resetting the private field data pointing back to the wrapper");
    //Make sure everything is at least initialized to 0.
    AAX_ASSERT(iDataSize == sizeof(SAAX_Wrapper_PrivateData));
    memset(oData, 0, iDataSize);

    //Set all of the private data variables.
    SAAX_Wrapper_PrivateData* privatedata = static_cast<SAAX_Wrapper_PrivateData*>(oData);
    privatedata->wrapper = (ClapAsAAX*)this;  // wrap away the weird const of the function declaration
    return AAX_SUCCESS;
  }

  //Call into the base class to clear all other private data.
  return AAX_CEffectParameters::ResetFieldData(iFieldIndex, oData, iDataSize);
}

AAX_Result ClapAsAAX::TimerWakeup()
{
  // Clear the used parameter changes
  // DeleteUsedParameterChanges();

  // note: this is neither mainthread nor audiothread
  return AAX_CEffectParameters::TimerWakeup();
}

AAX_Result ClapAsAAX::GetParameterIsAutomatable(AAX_CParamID iParameterID, AAX_CBoolean* itIs) const
{
  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    auto& info = n->second->_clap_param_info;
    *itIs = (info.flags & CLAP_PARAM_IS_AUTOMATABLE);
    return AAX_SUCCESS;
  }
  return AAX_ERROR_INVALID_PARAMETER_ID;
}

AAX_Result ClapAsAAX::GetParameterNumberOfSteps(AAX_CParamID iParameterID, int32_t* aNumSteps) const
{
  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    auto& info = n->second->_clap_param_info;
    if (info.flags & CLAP_PARAM_IS_STEPPED)
    {
      // the number of steps if min=0 and max=1 is 2
      *aNumSteps = 1 + (info.max_value - info.min_value);
    }
    else
    {
      *aNumSteps = 0;  // 0 means discrete/continuous, see class AAX_CParameter
    }

    return AAX_SUCCESS;
  }
  return AAX_ERROR_INVALID_PARAMETER_ID;
}

AAX_Result ClapAsAAX::GetParameterValueString(AAX_CParamID iParameterID, AAX_IString* oValueString,
                                              int32_t iMaxLength) const
{
  return AAX_CEffectParameters::GetParameterValueString(iParameterID, oValueString, iMaxLength);
}

AAX_Result ClapAsAAX::GetParameterValueFromString(AAX_CParamID iParameterID, double* oValuePtr,
                                                  const AAX_IString& iValueString) const
{
  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    if (n->second->_ext_params->text_to_value(_plugin->_plugin, n->second->_clap_param_info.id,
                                              iValueString.Get(), oValuePtr))
    {
      return AAX_SUCCESS;
    }
    else
    {
      return AAX_ERROR_INVALID_STRING_CONVERSION;
    }
  }
  else
  {
    return AAX_ERROR_INVALID_PARAMETER_ID;
  }
}

AAX_Result ClapAsAAX::GetParameterStringFromValue(AAX_CParamID iParameterID, double value,
                                                  AAX_IString* valueString, int32_t maxLength) const
{
  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    char flomf[256];
    if (this->_plugin->_ext._params->value_to_text(_plugin->_plugin, n->second->_clap_param_info.id,
                                                   n->second->asClapValue(value), flomf, sizeof(flomf)))
    {
      *valueString = flomf;
      return AAX_SUCCESS;
    }
    else
      return AAX_ERROR_INVALID_STRING_CONVERSION;
  }
  return AAX_ERROR_INVALID_PARAMETER_ID;
}

AAX_Result ClapAsAAX::GetParameterName(AAX_CParamID iParameterID, AAX_IString* oName) const
{
  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    *oName = n->second->_names.front();
  }
  *oName = "";
  return AAX_SUCCESS;
}

AAX_Result ClapAsAAX::GetParameterNameOfLength(AAX_CParamID iParameterID, AAX_IString* oName,
                                               int32_t iNameLength) const
{
  AAX_Result aResult = AAX_ERROR_INVALID_STRING_CONVERSION;

  auto n = this->_parameterMap.find(iParameterID);
  if (n != _parameterMap.end())
  {
    auto& names = n->second->_names;
    const AAX_CString* result = &names.back();
    for (auto i = names.rbegin(); i != names.rend(); ++i)
    {
      if (i->Length() > iNameLength)
      {
        oName->Set(result->StdString().c_str());
        return AAX_SUCCESS;
      }
      result = &(*i);
    }
    oName->Set(result->StdString().c_str());
    return AAX_SUCCESS;
  }
  return aResult;
}

static const AAX_CTypeID CLAP_STATE_CHUNK_ID = 'clap';

AAX_Result ClapAsAAX::GetNumberOfChunks(int32_t* oNumChunks) const
{
  // TODO: Return 1 (and only 1) chunk
  // return AAX_CEffectParameters::GetNumberOfChunks(oNumChunks);
  *oNumChunks = 1;
  return AAX_SUCCESS;
}

AAX_Result ClapAsAAX::GetChunkIDFromIndex(int32_t iIndex, AAX_CTypeID* oChunkID) const
{
  if (iIndex != 0)
  {
    *oChunkID = AAX_CTypeID(0);
    return AAX_ERROR_INVALID_CHUNK_INDEX;
  }

  *oChunkID = CLAP_STATE_CHUNK_ID;
  return AAX_SUCCESS;
}

AAX_Result ClapAsAAX::GetChunkSize(AAX_CTypeID iChunkID, uint32_t* oSize) const
{
  if (iChunkID != CLAP_STATE_CHUNK_ID) return AAX_ERROR_INVALID_CHUNK_ID;

  // This method is invoked every time a chunk is saved, therefore it is possible to have dynamically sized chunks.
  // However, note that each call to GetChunkSize() will correspond to a following call to GetChunk().
  // The chunk provided in GetChunk() must have the same size as the size provided by GetChunkSize().

  _state.clear();
  if (_plugin->_ext._state->save(_plugin->_plugin, _state))
  {
    *oSize = static_cast<uint32_t>(_state.size());
    return AAX_SUCCESS;
  }

  return AAX_ERROR_INCORRECT_CHUNK_SIZE;
}

AAX_Result ClapAsAAX::GetChunk(AAX_CTypeID iChunkID, AAX_SPlugInChunk* oChunk) const
{
  // Fills a block of data with chunk information representing the plug-in's current state.

  // By calling this method, the host is requesting information about the current state of the plug-in. The following chunk fields should be explicitly populated in this method. Other fields will be populated by the host.
  //
  // AAX_SPlugInChunk::fData
  // AAX_SPlugInChunk::fVersion
  // AAX_SPlugInChunk::fName (Optional)
  // AAX_SPlugInChunk::fSize (Data size only)

  if (iChunkID != CLAP_STATE_CHUNK_ID) return AAX_ERROR_INVALID_CHUNK_ID;

  oChunk->fVersion = 1;
  memset(oChunk->fName, 0, 32);  //Just in case, lets make sure unused chars are null.
  memcpy(oChunk->fName, "clap-as-aax", 11);
  oChunk->fSize = _state.size();
  memcpy(oChunk->fData, _state.data(), _state.size());

  return AAX_SUCCESS;
}

AAX_Result ClapAsAAX::SetChunk(AAX_CTypeID iChunkID, const AAX_SPlugInChunk* iChunk)
{
  if (iChunkID != CLAP_STATE_CHUNK_ID) return AAX_ERROR_INVALID_CHUNK_ID;

  auto data = (const uint8_t*)(iChunk->fData);
  _state.setData(data, iChunk->fSize);
  if (_plugin->_ext._state->load(_plugin->_plugin, _state))
  {
    return AAX_SUCCESS;
  }
  return AAX_ERROR_MALFORMED_CHUNK;
}

void ClapAsAAX::setupWrapperSpecifics(const clap_plugin_t* plugin)
{
}

void ClapAsAAX::setupAudioBusses(const clap_plugin_t* plugin,
                                 const clap_plugin_audio_ports_t* audioports)
{
}

void ClapAsAAX::setupMIDIBusses(const clap_plugin_t* plugin, const clap_plugin_note_ports_t* noteports)
{
}

void ClapAsAAX::setupParameters(const clap_plugin_t* plugin, const clap_plugin_params_t* params)
{
  if (!params) return;

  auto numparams = params->count(plugin);

  for (decltype(numparams) i = 0; i < numparams; ++i)
  {
    clap_param_info info;
    if (params->get_info(plugin, i, &info))
    {
      std::string paramname;

      if (info.module[0])
      {
        paramname = info.module;
        paramname.push_back('/');
      }
      paramname.append(info.name);

      auto id = createAAXId(info.id);

      auto wrappedParam = std::make_unique<AAXWrappedParameterInfo_t>(this, info, id);

      auto n = generateShortStrings(paramname);
      wrappedParam->_names.reserve(n.size());
      for (const auto& i : n)
      {
        wrappedParam->_names.emplace_back(AAX_CString(i));
      }

      // now to the map
      _parameterMap[id] = std::move(wrappedParam);
      auto p =
          new AAX_CParameter<double>(_parameterMap[id]->_aax_identifier.c_str(), AAX_CString(paramname),
                                     info.default_value, AAX_CLinearTaperDelegate<double>(0, 1),
                                     AAX_CUnitDisplayDelegateDecorator<double>(
                                         AAX_CNumberDisplayDelegate<double>(), AAX_CString(paramname)),
                                     info.flags & CLAP_PARAM_IS_AUTOMATABLE);
      mParameterManager.AddParameter(p);

#if 0
			auto* p = createParameter(info);
			if ( p )
				mParameterManager.AddParameter(p);
      //auto p = Vst3Parameter::create(
      //    &info, [&](const char* modstring) { return this->getOrCreateUnitInfo(modstring); });
      //// auto p = Vst3Parameter::create(&info,nullptr);
      //p->param_index_for_clap_get_info = i;
      //parameters.addParameter(p);
#endif
    }
  }
}

void ClapAsAAX::param_rescan(clap_param_rescan_flags flags)
{
}

void ClapAsAAX::param_clear(clap_id param, clap_param_clear_flags flags)
{
}

void ClapAsAAX::param_request_flush()
{
}

bool ClapAsAAX::gui_can_resize()
{
  return false;
}

bool ClapAsAAX::gui_request_resize(uint32_t width, uint32_t height)
{
  return false;
}

bool ClapAsAAX::gui_request_show()
{
  return false;
}

bool ClapAsAAX::gui_request_hide()
{
  return false;
}

void ClapAsAAX::latency_changed()
{
}

void ClapAsAAX::tail_changed()
{
}

void ClapAsAAX::mark_dirty()
{
}

void ClapAsAAX::restartPlugin()
{
}

void ClapAsAAX::request_callback()
{
}

bool ClapAsAAX::register_timer(uint32_t period_ms, clap_id* timer_id)
{
  return false;
}

bool ClapAsAAX::unregister_timer(clap_id timer_id)
{
  return false;
}

const char* ClapAsAAX::host_get_name()
{
  AAX_IController* ctrl = Controller();
  AAX_CString hostname;
  if (AAX_SUCCESS == ctrl->GetHostName(&hostname))
  {
    _wrapper_hostname = hostname.StdString();
    _wrapper_hostname.append(" (CLAP-as-AAX)");
  }
  return _wrapper_hostname.c_str();
}

bool ClapAsAAX::supportsContextMenu() const
{
  return false;
}

bool ClapAsAAX::context_menu_populate(const clap_context_menu_target_t* target,
                                      const clap_context_menu_builder_t* builder)
{
  return false;
}

bool ClapAsAAX::context_menu_perform(const clap_context_menu_target_t* target, clap_id action_id)
{
  return false;
}

bool ClapAsAAX::context_menu_can_popup()
{
  return false;
}

bool ClapAsAAX::context_menu_popup(const clap_context_menu_target_t* target, int32_t screen_index,
                                   int32_t x, int32_t y)
{
  return false;
}

void ClapAsAAX::onBeginEdit(clap_id id)
{
}

void ClapAsAAX::onPerformEdit(const clap_event_param_value_t* value)
{
}

void ClapAsAAX::onEndEdit(clap_id id)
{
}
