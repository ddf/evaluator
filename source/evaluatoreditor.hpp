#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "expression.hpp"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Compartmental {
    namespace Vst {
        
        class EvaluatorEditor: public VSTGUIEditor,
                               public IControlListener,
                               public IParameterFinder,
                               public IContextMenuTarget
        {
        public:
            //------------------------------------------------------------------------
            EvaluatorEditor (void* controller);
            
            //---from VSTGUIEditor---------------
            bool PLUGIN_API open (void* parent, const PlatformType& platformType = kDefaultNative);
            void PLUGIN_API close ();
            CMessageResult notify (CBaseObject* sender, const char* message);
            void beginEdit (long /*index*/) {}
            void endEdit (long /*index*/) {}
            
            //---from CControlListener---------
            void valueChanged (CControl* pControl);
            int32_t controlModifierClicked (CControl* pControl, CButtonState buttonState);
            void controlBeginEdit (CControl* pControl);
            void controlEndEdit (CControl* pControl);
            
            //---from EditorView---------------
            tresult PLUGIN_API onSize (ViewRect* newSize);
            tresult PLUGIN_API canResize () { return kResultFalse; }
            tresult PLUGIN_API checkSizeConstraint (ViewRect* rect);
            
            //---from IParameterFinder---------------
            tresult PLUGIN_API findParameter (int32 xPos, int32 yPos, ParamID& resultTag);
            
            //---from IContextMenuTarget---------------
            tresult PLUGIN_API executeMenuItem (int32 tag);
            
            DELEGATE_REFCOUNT (VSTGUIEditor)    
            tresult PLUGIN_API queryInterface (const char* iid, void** obj);
            
            //---Internal Function------------------
            void update (ParamID tag, ParamValue value);
            void messageTextChanged ();
            
        protected:
            CBitmap*    background;
            CTextLabel* timeLabel;
            CTextLabel* millisLabel;
            CTextLabel* rangeLabel;
            CTextLabel* noteLabel;
            CTextEdit* textEdit;
            CTextLabel* textResult;
            CSlider*   volumeSlider;
            CTextLabel* bitDepthLabel;
            CSlider*   bitDepthSlider;
            
            // used to validate the expression before transmitting the string to the controller.
            Expression expression;
        };
        //------------------------------------------------------------------------
    } // namespace Vst
} // namespace Compartmental
