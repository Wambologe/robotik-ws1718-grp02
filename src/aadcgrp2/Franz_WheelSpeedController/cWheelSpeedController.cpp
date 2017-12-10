/**
Copyright (c)
Audi Autonomous Driving Cup. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3.  All advertising materials mentioning features or use of this software must display the following acknowledgement: �This product includes software developed by the Audi AG and its contributors for Audi Autonomous Driving Cup.�
4.  Neither the name of Audi nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY AUDI AG AND CONTRIBUTORS �AS IS� AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL AUDI AG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


**********************************************************************
* $Author:: spiesra $  $Date:: 2017-05-12 10:01:55#$ $Rev:: 63111   $
**********************************************************************/


// arduinofilter.cpp : Definiert die exportierten Funktionen f�r die DLL-Anwendung.
//
#include "stdafx.h"
#include "cWheelSpeedController.h"

#define WSC_PROP_PID_KP "PID::Kp_value"
#define WSC_PROP_PID_KI "PID::Ki_value"
#define WSC_PROP_PID_KD "PID::Kd_value"
#define WSC_PROP_PID_SAMPLE_TIME "PID::Sample_Interval_[msec]"

// id and name definitions for signal registry (increase the id for new signals)
#define WSC_SIGREG_ID_WHEELSPEED_SETVALUE 0
#define WSC_SIGREG_NAME_WHEELSPEED_SETVALUE "wheel speed set value"
#define WSC_SIGREG_UNIT_WHEELSPEED_SETVALUE "m/sec2"

#define WSC_SIGREG_ID_WHEELSPEED_MEASVALUE 1
#define WSC_SIGREG_NAME_WHEELSPEED_MEASVALUE "wheel speed measured value"
#define WSC_SIGREG_UNIT_WHEELSPEED_MEASVALUE "m/sec2"

#define WSC_SIGREG_ID_WHEELSPEED_OUTPUTVALUE 2
#define WSC_SIGREG_NAME_WHEELSPEED_OUTPUTVALUE "wheel speed output value"
#define WSC_SIGREG_UNIT_WHEELSPEED_OUTPUTVALUE "m/sec2"

#define WSC_PROP_PID_MAXOUTPUT "PID::Maxiumum output"
#define WSC_PROP_PID_MINOUTPUT "PID::Minimum output"
#define WSC_PROP_DEBUG_MODE "Debug Mode"


ADTF_FILTER_PLUGIN("Franz Wheel Speed Controller", OID_ADTF_WHEELSPEEDCONTROLLER, cWheelSpeedController)

cWheelSpeedController::cWheelSpeedController(const tChar* __info) : cFilter(__info), m_f64LastMeasuredError(0), m_f64SetPoint(0), m_lastSampleTime(0),m_f64LastSpeedValue(0)
{
    SetPropertyFloat(WSC_PROP_PID_KP,1.1);
    SetPropertyBool(WSC_PROP_PID_KP NSSUBPROP_ISCHANGEABLE, tTrue);
    SetPropertyStr(WSC_PROP_PID_KP NSSUBPROP_DESCRIPTION, "The proportional factor Kp for the PID Controller");

    SetPropertyFloat(WSC_PROP_PID_KI,0.001);
    SetPropertyBool(WSC_PROP_PID_KI NSSUBPROP_ISCHANGEABLE, tTrue);
    SetPropertyStr(WSC_PROP_PID_KI NSSUBPROP_DESCRIPTION, "The integral factor Ki for the PID Controller");

    SetPropertyFloat(WSC_PROP_PID_KD,1);
    SetPropertyBool(WSC_PROP_PID_KD NSSUBPROP_ISCHANGEABLE, tTrue);
    SetPropertyStr(WSC_PROP_PID_KD NSSUBPROP_DESCRIPTION, "The differential factor Kd for the PID Controller");

    SetPropertyFloat(WSC_PROP_PID_MAXOUTPUT,10);
    SetPropertyBool(WSC_PROP_PID_MAXOUTPUT NSSUBPROP_ISCHANGEABLE, tTrue);
    SetPropertyStr(WSC_PROP_PID_MAXOUTPUT NSSUBPROP_DESCRIPTION, "The maximum allowed output for the wheel speed controller (speed in m/sec^2)");

    SetPropertyFloat(WSC_PROP_PID_MINOUTPUT,-10);
    SetPropertyBool(WSC_PROP_PID_MINOUTPUT NSSUBPROP_ISCHANGEABLE, tTrue);
    SetPropertyStr(WSC_PROP_PID_MINOUTPUT NSSUBPROP_DESCRIPTION, "The minimum allowed output for the wheel speed controller (speed in m/sec^2)");

    SetPropertyBool(WSC_PROP_DEBUG_MODE, tFalse);
    SetPropertyStr(WSC_PROP_DEBUG_MODE NSSUBPROP_DESCRIPTION, "If true debug infos are written to output");

    //m_pISignalRegistry = NULL;

    m_f64LastOutput = 0;
    m_f64LastMeasuredError = 0;
    m_f64SetPoint = 0;
    m_lastSampleTime = GetTime();
    m_f64LastSpeedValue = 0;
    m_f64accumulatedVariable = 0;
}

cWheelSpeedController::~cWheelSpeedController()
{
}


tResult cWheelSpeedController::PropertyChanged(const char* strProperty)
{
    ReadProperties(strProperty);

    RETURN_NOERROR;
}

tResult cWheelSpeedController::ReadProperties(const tChar* strPropertyName)
{

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_PID_KP))
    {
        m_f64PIDKp = GetPropertyFloat(WSC_PROP_PID_KP);
    }

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_PID_KD))
    {
        m_f64PIDKd = GetPropertyFloat(WSC_PROP_PID_KD);
    }

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_PID_KI))
    {
        m_f64PIDKi = GetPropertyFloat(WSC_PROP_PID_KI);
    }

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_DEBUG_MODE))
    {
        m_bShowDebug = static_cast<tBool>(GetPropertyBool(WSC_PROP_DEBUG_MODE));
    }

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_PID_MINOUTPUT))
    {
        m_f64PIDMinimumOutput = GetPropertyFloat(WSC_PROP_PID_MINOUTPUT);
    }

    if (NULL == strPropertyName || cString::IsEqual(strPropertyName, WSC_PROP_PID_MAXOUTPUT))
    {
        m_f64PIDMaximumOutput = GetPropertyFloat(WSC_PROP_PID_MAXOUTPUT);
    }

    RETURN_NOERROR;
}


tResult cWheelSpeedController::CreateInputPins(__exception)
{
    // create description manager
    cObjectPtr<IMediaDescriptionManager> pDescManager;
    RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));

    // get media tayp
    tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
    RETURN_IF_POINTER_NULL(strDescSignalValue);
    cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);

    // set member media description
    RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pDescMeasSpeed));
    RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pDescSetSpeed));

    // create pins
    RETURN_IF_FAILED(m_oInputSetWheelSpeed.Create("set_WheelSpeed", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
    RETURN_IF_FAILED(RegisterPin(&m_oInputSetWheelSpeed));
    RETURN_IF_FAILED(m_oInputMeasWheelSpeed.Create("measured_wheelSpeed", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
    RETURN_IF_FAILED(RegisterPin(&m_oInputMeasWheelSpeed));

    RETURN_NOERROR;
}

tResult cWheelSpeedController::CreateOutputPins(__exception)
{
    // create description manager
    cObjectPtr<IMediaDescriptionManager> pDescManager;
    RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));

    // get media tayp
    tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
    RETURN_IF_POINTER_NULL(strDescSignalValue);
    cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);

    // set member media description
    RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pDescActuator));

    // create pin
    RETURN_IF_FAILED(m_oOutputActuator.Create("actuator_output", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
    RETURN_IF_FAILED(RegisterPin(&m_oOutputActuator));

    RETURN_NOERROR;
}

tResult cWheelSpeedController::GetInterface(const tChar* idInterface,
        tVoid** ppvObject)
{

    return cFilter::GetInterface(idInterface, ppvObject);

    Ref();

    RETURN_NOERROR;
}

tResult cWheelSpeedController::Init(tInitStage eStage, __exception)
{
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr))

    if (eStage == StageFirst)
    {
        RETURN_IF_FAILED(CreateInputPins(__exception_ptr));
        RETURN_IF_FAILED(CreateOutputPins(__exception_ptr));
    }
    else if (eStage == StageNormal)
    {
        ReadProperties(NULL);

        if (m_bShowDebug)
        {
            printf("Speed Controller started\n");
        }
    }
    else if(eStage == StageGraphReady)
    {
        // set the flags which indicate if the media descriptions strings were set
        m_bInputMeasWheelSpeedGetID = tFalse;
        m_bInputSetWheelSpeedGetID = tFalse;
        m_bInputActuatorGetID = tFalse;

        m_f64LastOutput = 0.0f;
    }

    RETURN_NOERROR;
}

tResult cWheelSpeedController::Start(__exception)
{
    m_f64LastOutput = 0;
    m_f64LastMeasuredError = 0;
    m_f64SetPoint = 0;
    m_lastSampleTime = GetTime();
    m_f64LastSpeedValue = 0;
    m_f64accumulatedVariable = 0;

    return cFilter::Start(__exception_ptr);
}

tResult cWheelSpeedController::Stop(__exception)
{
    return cFilter::Stop(__exception_ptr);
}

tResult cWheelSpeedController::Shutdown(tInitStage eStage, __exception)
{
    if (eStage == StageNormal)
    {
        m_oActive.clear();

        m_oLock.Release();
    }
    return cFilter::Shutdown(eStage,__exception_ptr);
}

tResult cWheelSpeedController::OnPinEvent(    IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{
    __synchronized_obj(m_critSecOnPinEvent);
    if (nEventCode == IPinEventSink::PE_MediaSampleReceived && pMediaSample != NULL)
    {
        RETURN_IF_POINTER_NULL( pMediaSample);
        if (pSource == &m_oInputMeasWheelSpeed)
        {

            //write values with zero
            tFloat32 f32Value = 0;
            tUInt32 Ui32TimeStamp = 0;
            {
                // focus for sample write lock
                //read data from the media sample with the coder of the descriptor
                __adtf_sample_read_lock_mediadescription(m_pDescMeasSpeed,pMediaSample,pCoder);

                if(!m_bInputMeasWheelSpeedGetID)
                {
                    pCoder->GetID("f32Value", m_buIDMeasSpeedF32Value);
                    pCoder->GetID("ui32ArduinoTimestamp", m_buIDMeasSpeedArduinoTimestamp);
                    m_bInputMeasWheelSpeedGetID = tTrue;
                }
                //get values from media sample
                pCoder->Get(m_buIDMeasSpeedF32Value, (tVoid*)&f32Value);
                pCoder->Get(m_buIDMeasSpeedArduinoTimestamp, (tVoid*)&Ui32TimeStamp);
            }

            // write to member variable

            m_f64MeasuredVariable = f32Value;

            //calculation
            // if speed = 0 is requested output is immediately set to zero
            if (m_f64SetPoint==0) {
                m_f64LastOutput = 0;
                m_f64accumulatedVariable = 0;
                m_f64LastMeasuredError = 0;
                m_lastSampleTime = GetTime();
            }
            else {
                m_f64LastOutput= getControllerValue(f32Value);
            }

            //create new media sample
            cObjectPtr<IMediaSample> pNewMediaSample;
            AllocMediaSample((tVoid**)&pNewMediaSample);

            //allocate memory with the size given by the descriptor
            cObjectPtr<IMediaSerializer> pSerializer;
            m_pDescActuator->GetMediaSampleSerializer(&pSerializer);
            tInt nSize = pSerializer->GetDeserializedSize();
            pNewMediaSample->AllocBuffer(nSize);
            tFloat32 outputValue = static_cast<tFloat32>(m_f64LastOutput);
            {
                // focus for sample write lock
                //read data from the media sample with the coder of the descriptor
                __adtf_sample_write_lock_mediadescription(m_pDescActuator,pNewMediaSample,pCoderOut);

                if(!m_bInputActuatorGetID)
                {
                    pCoderOut->GetID("f32Value", m_buIDActuatorF32Value);
                    pCoderOut->GetID("ui32ArduinoTimestamp", m_buIDActuatorArduinoTimestamp);
                    m_bInputActuatorGetID = tTrue;
                }
                //get values from media sample
                pCoderOut->Set(m_buIDActuatorF32Value, (tVoid*)&outputValue);
            }

            //transmit media sample over output pin
            RETURN_IF_FAILED(pNewMediaSample->SetTime(_clock->GetStreamTime()));
            RETURN_IF_FAILED(m_oOutputActuator.Transmit(pNewMediaSample));
        }
        else if (pSource == &m_oInputSetWheelSpeed)
        {
            {
                //write values with zero
                tFloat32 f32Value = 0;
                tUInt32 ui32TimeStamp = 0;

                // focus for sample write lock
                __adtf_sample_read_lock_mediadescription(m_pDescSetSpeed,pMediaSample,pCoder);

                if(!m_bInputSetWheelSpeedGetID)
                {
                    pCoder->GetID("f32Value", m_buIDSetSpeedF32Value);
                    pCoder->GetID("ui32ArduinoTimestamp", m_buIDSetSpeedArduinoTimestamp);
                    m_bInputSetWheelSpeedGetID = tTrue;
                }

                // read data from the media sample with the coder of the descriptor
                //get values from media sample
                pCoder->Get(m_buIDSetSpeedF32Value, (tVoid*)&f32Value);
                pCoder->Get(m_buIDSetSpeedArduinoTimestamp, (tVoid*)&ui32TimeStamp);

                // write to member variable
                m_f64SetPoint = static_cast<tFloat64>(f32Value);
            }
        }
    }
    RETURN_NOERROR;
}



tFloat64 cWheelSpeedController::getControllerValue(tFloat64 i_f64MeasuredValue)
{

    //i_f64MeasuredValue = (i_f64MeasuredValue +  m_f64LastSpeedValue) /2.0;

    //m_f64LastSpeedValue = i_f64MeasuredValue;

    // calculate time delta since last sample
    tTimeStamp deltaT = GetTime() - m_lastSampleTime;
    m_lastSampleTime = GetTime();

    if (m_bShowDebug) {
      std::cout << "deltaT -> " <<  deltaT << '\n';
      std::cout << "current Speed -> " << i_f64MeasuredValue << '\n';
      std::cout << "Set Value: " << m_f64SetPoint << " Measured Value: " << i_f64MeasuredValue << '\n';
    }

    tFloat f64Result = 0;

    //error:
    tFloat64 f64Error = (m_f64SetPoint - i_f64MeasuredValue);

    //algorithm:
    //esum = esum + e
    //y = Kp * e + Ki * Ta * esum + Kd * (e � ealt)/Ta
    //ealt = e

    // accumulated error:
    m_f64accumulatedVariable += f64Error * deltaT / 1e6;

    tFloat64 y_p = m_f64PIDKp * f64Error;
    tFloat64 y_i = m_f64PIDKi * m_f64accumulatedVariable;
    tFloat64 y_d = m_f64PIDKd * (f64Error - m_f64LastMeasuredError) / deltaT;

    f64Result =  y_p + y_i + y_d;

    m_f64LastMeasuredError = f64Error;

    if (m_bShowDebug) {
      std::cout << "Error: " << f64Error << " Accumulated Error " << m_f64accumulatedVariable << '\n';
    }


    if (m_bShowDebug) {
      std::cout << "Output Value before limit " << f64Result  << '\n';
    }

    // checking for minimum and maximum limits
    if (f64Result>m_f64PIDMaximumOutput)
    {
        f64Result = m_f64PIDMaximumOutput;
    }
    else if (f64Result<m_f64PIDMinimumOutput)
    {
        f64Result = m_f64PIDMinimumOutput;
    }

    m_f64LastOutput = f64Result;

    if (m_bShowDebug) {
      std::cout << "Output Value after limit " << f64Result  << '\n';
    }

    return f64Result;
}


tTimeStamp cWheelSpeedController::GetTime()
{
    return (_clock != NULL) ? _clock->GetTime () : cSystem::GetTime();
}


tUInt cWheelSpeedController::Ref()
{
    return cFilter::Ref();
}

tUInt cWheelSpeedController::Unref()
{
    return cFilter::Unref();
}

tVoid cWheelSpeedController::Destroy()
{
    delete this;
}