#define PLUG_MFR "Damien Quartz"
#define PLUG_NAME "Evaluator"

#define PLUG_CLASS_NAME Evaluator

#define BUNDLE_MFR "compartmental"
#define BUNDLE_NAME "Evaluator"

#define PLUG_ENTRY Evaluator_Entry
#define PLUG_VIEW_ENTRY Evaluator_ViewEntry

#define PLUG_ENTRY_STR "Evaluator_Entry"
#define PLUG_VIEW_ENTRY_STR "Evaluator_ViewEntry"

#define VIEW_CLASS Evaluator_View
#define VIEW_CLASS_STR "Evaluator_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'Eval'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Cmpt'

// ProTools stuff

#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'EFN1', 'EFN2'};
  const int PLUG_TYPE_IDS_AS[2] = {'EFA1', 'EFA2'}; // AudioSuite
#endif

#define PLUG_MFR_PT "Damien Quartz\nDamien Quartz\nCompartmental"
#define PLUG_NAME_PT "Evaluator\nEVAL"
#define PLUG_TYPE_PT "Effect"
#define PLUG_DOES_AUDIOSUITE 1

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 1

#define PLUG_DOES_STATE_CHUNKS 1

// Unique IDs for each image resource.
#define BACKGROUND_ID 101
#define SLIDER_BACK_ID 102
#define SLIDER_KNOB_ID 103
#define NUMBERBOX_ARROW_UP_ID 104
#define NUMBERBOX_ARROW_DOWN_ID 105
#define NUMBERBOX_BACK_ID 106
#define RADIO_BUTTON_ID 107

// Image resource locations for this plug.
#define BACKGROUND_FN "resources/img/background.png"
#define SLIDER_BACK_FN "resources/img/vslider_background.png"
#define SLIDER_KNOB_FN "resources/img/vslider_handle.png"
#define NUMBERBOX_ARROW_UP_FN "resources/img/numberbox_arrow_up.png"
#define NUMBERBOX_ARROW_DOWN_FN "resources/img/numberbox_arrow_down.png"
#define NUMBERBOX_BACK_FN "resources/img/numberbox_background.png"
#define RADIO_BUTTON_FN "resources/img/radio_button.png"

// GUI default dimensions
#define GUI_WIDTH 640
#define GUI_HEIGHT 640

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API)
#include "app_wrapper/app_resource.h"
#endif

// vst3 stuff
#define MFR_URL "www.compartmental.net"
#define MFR_EMAIL "info@compartmental.net"
#define EFFECT_TYPE_VST3 "Fx|Generator"

/* "Fx|Analyzer"", "Fx|Delay", "Fx|Distortion", "Fx|Dynamics", "Fx|EQ", "Fx|Filter",
"Fx", "Fx|Instrument", "Fx|InstrumentExternal", "Fx|Spatial", "Fx|Generator",
"Fx|Mastering", "Fx|Modulation", "Fx|PitchShift", "Fx|Restoration", "Fx|Reverb",
"Fx|Surround", "Fx|Tools", "Instrument", "Instrument|Drum", "Instrument|Sampler",
"Instrument|Synth", "Instrument|Synth|Sampler", "Instrument|External", "Spatial",
"Spatial|Fx", "OnlyRT", "OnlyOfflineProcess", "Mono", "Stereo",
"Surround"
*/
