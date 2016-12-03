//-----------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.6.6
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/adelayprocessor.cpp
// Created by  : Steinberg, 06/2009
// Description : 
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2016, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "evaluatorprocessor.h"
#include "expression.hpp"

#include "evaluatorids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <algorithm>

namespace Compartmental
{
    namespace Vst
    {
        static float bd(float val)
        {
            return val/(kBitDepthMax+1);
        }
        
        const EvaluatorProcessor::Preset EvaluatorProcessor::presets[] =
        {
            // name, expression, volume, bit depth
            { "saw wave", "t*fn", 0.1f, bd(15.0f) },
            { "square wave", "#(t*fn)", 0.1f, bd(15.0f) },
            { "sine wave", "$(t*fn)", 0.1f, bd(15.0f) },
            { "triangle wave", "(t*fn)*((t*fn/r)%2) + (r-t*fn-1)*(1 - (t*fn/r)%2)", 0.1f, bd(13.0f) },
            { "amplitude modulation", "t*fn | $(t)", 0.1f, bd(15.0f) },
            { "frequency modulation", "t*fn + $(t*2)", 0.1f, bd(15.0f) },
            { "bouncing balls", "$(t*(1000 - m%500))", 0.1f, bd(15.0f) },
            { "little ditty", "(t*128 + $(t)) | t>>(t%(8*r))/r | t>>128", 0.1f, bd(15.0f) },
            { "aggressive texture", "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", 0.1f, bd(15.0f) },
            { "overtone waterfall", "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", 0.1f, bd(17.0f) },
            { "computer music" , "$(t*f(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", 0.1f, bd(11.0f) },
            { "blurp", "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", 0.1f, bd(15.0f) },
            { "garbage trash", "(r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", 0.1f, bd(15.0f) },
            { "nonsense can", "(1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", 0.1f, bd(15.0f) },
            { "ellipse", "(m/250+1)*$(t*128) | (m/500+1)*$((t+r/2*128))", 0.1f, bd(12.0f) },
            { "moving average", "p + ( ((t+1)*256 ^ (t+1)*64 & (t+1)*32) - p)/(t+1)", 0.1f, bd(15.0f) },
            { "oink oink ribbit", "(t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", 0.3f, bd(18.0f) },
            { "rhythmic glitch sine", "$(t*fn) | t*n/10>>4 ^ p>>(m/250%12)", 0.15f, bd(13.0f) }
        };
        
        const int32 EvaluatorProcessor::numPresets = sizeof(EvaluatorProcessor::presets)/sizeof(EvaluatorProcessor::Preset);
        
        // helper for adding output parameters for display in the gui
        static void addOutParam(IParameterChanges* outParams, ParamID id, double value)
        {
            int32 index = 0;
            IParamValueQueue* paramQueue = outParams->addParameterData (id, index);
            if (paramQueue)
            {
                int32 index2 = 0;
                paramQueue->addPoint (0, value, index2);
            }
        }

        //-----------------------------------------------------------------------------
        EvaluatorProcessor::EvaluatorProcessor ()
        : mVolume(0.1f)
        , mEvaluator(0)
        , mExpression("")
        , mNoteOnPitch(-1)
        , mNoteOnVelocity(0)
        , mBypass (false)
        , mBitDepth(0.4f)
        , mTick(0)
        {
            setControllerClass (EvaluatorControllerUID);
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::initialize (FUnknown* context)
        {
            tresult result = AudioEffect::initialize (context);
            if (result == kResultTrue)
            {
                addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
                addAudioOutput (STR16 ("AudioOutput"), SpeakerArr::kStereo);
                
                //---create Event In/Out buses (1 bus with only 1 channel)------
                // this gets us midi events
                addEventInput (STR16 ("Event In"), 1);
            }
            return result;
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
        {
            // we only support one in and output bus and these buses must have the same number of channels
            if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
                return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
            return kResultFalse;
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setActive (TBool state)
        {
            SpeakerArrangement arr;
            if (getBusArrangement (kOutput, 0, arr) != kResultTrue)
                return kResultFalse;
            int32 numChannels = SpeakerArr::getChannelCount (arr);
            if (numChannels == 0)
                return kResultFalse;

            if (state)
            {
                mEvaluator = new Expression();
                mEvaluator->Compile(mExpression);
            }
            else
            {
                if (mEvaluator)
                {
                    delete mEvaluator;
                    mEvaluator = 0;
                }
            }
            return AudioEffect::setActive (state);
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::process (ProcessData& data)
        {
            if (data.inputParameterChanges)
            {
                int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
                for (int32 index = 0; index < numParamsChanged; index++)
                {
                    IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData (index);
                    if (paramQueue)
                    {
                        ParamValue value;
                        int32 sampleOffset;
                        int32 numPoints = paramQueue->getPointCount ();
                        switch (paramQueue->getParameterId ())
                        {
                            case kVolumeId:
                            {
                                if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    mVolume = value;
                                }
                            }
                            break;
                                
                            case kBypassId:
                            {
                                if (paramQueue->getPoint (numPoints - 1,  sampleOffset, value) == kResultTrue)
                                {
                                    mBypass = (value > 0.5f);
                                }
                            }
                            break;
                                
                            case kBitDepthId:
                            {
                                if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    mBitDepth = value;
                                }
                            }
                            break;
                                
                            case kPresetId:
                            {
                                if ( paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    int32 program = std::min<int32> (numPresets-1, (int32)(value * numPresets));
                                    const Preset& preset = presets[program];
                                    receiveText(preset.expression);
                                    mVolume = preset.volume;
                                    mBitDepth = preset.bitDepth;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            
            // grab this now, mTick might change from note events
            const EvalValue startTick = mTick;
            const EvalValue startM = mEvaluator->GetVar('m');
            const EvalValue startR = mEvaluator->GetVar('r');
            const int32 startN = mEvaluator->GetVar('n');
            
            //---2) Read input events-------------
            IEventList* eventList = data.inputEvents;
            if (eventList)
            {
                int32 numEvent = eventList->getEventCount ();
                for (int32 i = 0; i < numEvent; i++)
                {
                    Event event;
                    if (eventList->getEvent (i, event) == kResultOk)
                    {
                        switch (event.type)
                        {
                            case Event::kNoteOnEvent:
                            {
                                mNoteOnPitch = event.noteOn.pitch;
                                mNoteOnVelocity = event.noteOn.velocity;
                                mTick = 0;
                                mEvaluator->SetVar('n', mNoteOnPitch);
                                mEvaluator->SetVar('p', 0);
                            }
                            break;
                                
                            case Event::kNoteOffEvent:
                            {
                                mNoteOnPitch = -1;
                                mTick = 0;
                            }
                            break;
                        }
                    }
                }
            }

            if (data.numSamples > 0)
            {
                SpeakerArrangement arr;
                getBusArrangement (kOutput, 0, arr);
                int32 numChannels = SpeakerArr::getChannelCount (arr);

                // the stepCount and shift calculations match how RangeParameter does it.
                // ensures that we use same value as is displayed in the UI
                const int32 stepCount = kBitDepthMax-kBitDepthMin;
                const int32 shift = std::min<int32> (stepCount, (int32)(mBitDepth * (stepCount + 1))) + kBitDepthMin;
                const EvalValue range = 1<<shift;
                const float amp = mNoteOnPitch >= 0 ? mVolume*mNoteOnVelocity : 0;
                const uint64 mdenom = (uint64)(processSetup.sampleRate/1000);
                const bool generate = amp > 0;
                const EvalValue p = mEvaluator->GetVar('p');
                
                mEvaluator->SetVar('r', range);
                
                for (uint32 channel = 0; channel < numChannels; channel++)
                {
                    float* inputChannel = data.inputs[0].channelBuffers32[channel];
                    float* outputChannel = data.outputs[0].channelBuffers32[channel];
                    
                    if ( channel > 0 ) mEvaluator->SetVar('p', p);
                    
                    for (uint32 sample = 0; sample < data.numSamples; sample++)
                    {
                        float evalSample = 0;
                        if ( generate )
                        {
                            EvalValue tempTick = mTick + sample;
                            mEvaluator->SetVar('t', tempTick);
                            mEvaluator->SetVar('m', tempTick/mdenom);
                            EvalValue result = mEvaluator->Eval();
                            mEvaluator->SetVar('p', result);
                            evalSample = amp * (float)(-1.0 + 2.0*((double)(result%range)/(range-1)) );
                        }
                        outputChannel[sample] = inputChannel[sample] + evalSample;
                    }
                }
                if ( mNoteOnPitch >= 0 )
                {
                    mTick += data.numSamples;
                }
            }
            
            //---3) Write outputs parameter changes-----------
            IParameterChanges* outParamChanges = data.outputParameterChanges;
            // a new value of VuMeter will be send to the host
            // (the host will send it back in sync to our controller for updating our editor)
            if ( outParamChanges )
            {
                if ( mTick != startTick )
                {
                    addOutParam(outParamChanges, kEvalTId, (double)mTick/kMaxInt64u);
                }
                
                if ( mEvaluator->GetVar('m') != startM )
                {
                    addOutParam(outParamChanges, kEvalMId, (double)mEvaluator->GetVar('m')/kMaxInt64u);
                }
                
                if ( mEvaluator->GetVar('r') != startR )
                {
                    addOutParam(outParamChanges, kEvalRId, (double)mEvaluator->GetVar('r')/kMaxInt64u);
                }
                
                if ( mEvaluator->GetVar('n') != startN )
                {
                    addOutParam(outParamChanges, kEvalNId, (double)(mNoteOnPitch+1)/kMaxInt32);
                }
            }
            
            return kResultTrue;
        }

        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setState (IBStream* state)
        {
            if (!state)
                return kResultFalse;

            // called when we load a preset, the model has to be reloaded

            float savedVolume = 0.f;
            if (state->read (&savedVolume, sizeof (float)) != kResultOk)
            {
                return kResultFalse;
            }

            int32 savedBypass = 0;
            if (state->read (&savedBypass, sizeof (int32)) != kResultOk)
            {
                // could be an old version, continue 
            }
            
            float savedBitDepth = 0.5f;
            if (state->read(&savedBitDepth, sizeof(float)) != kResultOk)
            {
                // could be old vesion, ok if not there
            }
            
            int8 byteOrder;
            if ((state->read (&byteOrder, sizeof (int8))) == kResultOk)
            {
                state->read (mExpression, sizeof(char)*128);
            
                // if the byteorder doesn't match, byte swap the text array ...
                if (byteOrder != BYTEORDER)
                {
                    for (int32 i = 0; i < 128; i++)
                    {
                        SWAP_16 (mExpression[i])
                    }
                }
                
                if ( mEvaluator )
                {
                    mEvaluator->Compile(mExpression);
                }
            }

            #if BYTEORDER == kBigEndian
            SWAP_32 (savedVolume)
            SWAP_32 (savedBypass)
            SWAP_32 (savedBitDepth)
            #endif

            mVolume = savedVolume;
            mBypass = savedBypass > 0;
            mBitDepth = savedBitDepth;
            
            

            return kResultOk;
        }

        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::getState (IBStream* state)
        {
            // here we need to save the model

            float toSaveVolume = mVolume;
            int32 toSaveBypass = mBypass ? 1 : 0;
            float toSaveBitDepth = mBitDepth;

            #if BYTEORDER == kBigEndian
            SWAP_32 (toSaveVolume)
            SWAP_32 (toSaveBypass)
            SWAP_32 (toSaveBitDepth)
            #endif

            state->write (&toSaveVolume, sizeof (float));
            state->write (&toSaveBypass, sizeof (int32));
            state->write (&toSaveBitDepth, sizeof(float));
            
            // as we save a Unicode string, we must know the byteorder when setState is called
            int8 byteOrder = BYTEORDER;
            if (state->write (&byteOrder, sizeof (int8)) == kResultOk)
            {
                return state->write (mExpression, sizeof(char)*128);
            }
            return kResultFalse;

            return kResultOk;
        }
            
        //------------------------------------------------------------------------
        tresult EvaluatorProcessor::receiveText (const char* text)
        {
            // received from Controller
            fprintf (stderr, "[Evaluator] received: ");
            fprintf (stderr, text);
            fprintf (stderr, "\n");
            
            strncpy(mExpression, text, 127);
            
            if ( mEvaluator )
            {
                mEvaluator->Compile(mExpression);
            }
            
            return kResultOk;
        }

    } // namespace Vst
} // namespace Compartmental
