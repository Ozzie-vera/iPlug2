/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>


#include "wdlendian.h"

#include "IPlugAPIBase.h"

IPlugAPIBase::IPlugAPIBase(IPlugConfig c, EAPI plugAPI)
  : IPluginBase(c.nParams, c.nPresets)
  , mParamChangeFromProcessor(512) // TODO: CONSTANT
{
  mUniqueID = c.uniqueID;
  mMfrID = c.mfrID;
  mVersion = c.vendorVersion;
  mPluginName.Set(c.pluginName, MAX_PLUGIN_NAME_LEN);
  mProductName.Set(c.productName, MAX_PLUGIN_NAME_LEN);
  mMfrName.Set(c.mfrName, MAX_PLUGIN_NAME_LEN);
  mHasUI = c.plugHasUI;
  mWidth = c.plugWidth;
  mHeight = c.plugHeight;
  mStateChunks = c.plugDoesChunks;
  mAPI = plugAPI;

  Trace(TRACELOC, "%s:%s", c.pluginName, CurrentTime());
  
  mParamDisplayStr.Set("", MAX_PARAM_DISPLAY_LEN);
}

IPlugAPIBase::~IPlugAPIBase()
{
  if(mTimer)
  {
    mTimer->Stop();
    delete mTimer;
  }

  TRACE;
}

void IPlugAPIBase::OnHostRequestingImportantParameters(int count, WDL_TypedBuf<int>& results)
{
  for (int i = 0; i < count; i++)
    results.Add(i);
}

void IPlugAPIBase::CreateTimer()
{
  mTimer = Timer::Create(*this, IDLE_TIMER_RATE);
}

bool IPlugAPIBase::CompareState(const uint8_t* pIncomingState, int startPos)
{
  bool isEqual = true;
  
  const double* data = (const double*) pIncomingState + startPos;
  
  // dirty hack here because protools treats param values as 32 bit int and in IPlug they are 64bit float
  // if we memcmp() the incoming state with the current they may have tiny differences due to the quantization
  for (int i = 0; i < NParams(); i++)
  {
    float v = (float) GetParam(i)->Value();
    float vi = (float) *(data++);
    
    isEqual &= (fabsf(v - vi) < 0.00001);
  }
  
  return isEqual;
}

#pragma mark -

void IPlugAPIBase::PrintDebugInfo() const
{
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("\n--------------------------------------------------\n%s\n", buildInfo.Get());
}

#pragma mark -

void IPlugAPIBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;
  
  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
}

void IPlugAPIBase::SetParameterValue(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx, kGUI);
}

void IPlugAPIBase::OnParamReset(EParamSource source)
{
  for (int i = 0; i < mParams.GetSize(); ++i)
  {
    OnParamChange(i, source);
  }
}

void IPlugAPIBase::DirtyParameters()
{
  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugAPIBase::_SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized)
{
  //TODO: Can we assume that no host is stupid enough to try and set parameters on multiple threads at the same time?
  // If that is the case then we need a MPSPC queue not SPSC
  mParamChangeFromProcessor.Push(ParamChange { paramIdx, value, normalized } );
}

void IPlugAPIBase::OnTimer(Timer& t)
{
  if(HasUI())
  {
  #if !defined VST3C_API && !defined VST3P_API
    while(mParamChangeFromProcessor.ElementsAvailable())
    {
      ParamChange p;
      mParamChangeFromProcessor.Pop(p);
      SendParameterValueToUIFromDelegate(p.paramIdx, p.value, p.normalized); // TODO:  if the parameter hasn't changed maybe we shouldn't do anything?
    }
    
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      OnMidiMsgUI(msg);
    }
  #endif
    
  #if defined VST3P_API
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      _TransmitMidiMsgFromProcessor(msg);
    }
  #endif
  }
  
  OnIdle();
}

void IPlugAPIBase::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  mMidiMsgsFromEditor.Push(msg);
}

void IPlugAPIBase::SendSysexMsgFromUI(int size, const uint8_t* pData)
{
  //TODO:
}

void IPlugAPIBase::SendMsgFromUI(int messageTag, int dataSize, const void* pData)
{
  OnMessage(messageTag, dataSize, pData);
}
