#include "evaluatoreditor.hpp"
#include "evaluatorcontroller.h"
#include "evaluatorids.h"

#include "base/source/fstring.h"

#if WINDOWS
#include "../win/resource.h"
#define BACKGROUND IDB_PNG1
#define SLIDER_BACK IDB_PNG2
#define SLIDER_KNOB IDB_PNG3

#else
#define BACKGROUND "background.png"
#define SLIDER_BACK "vslider_background.png"
#define SLIDER_KNOB "vslider_handle.png"
#endif

static CFontDesc gLabelFont("Courier", 11, kBoldFace);
static CFontDesc gDataFont("Courier", 11);

static const CFontRef kLabelFont = &gLabelFont;
static const CFontRef kDataFont = &gDataFont;

namespace Compartmental {
    namespace Vst {
        
        //------------------------------------------------------------------------
        enum
        {
            // UI size
            kEditorWidth  = 500,
            kEditorHeight = 250
        };
        
        enum
        {
            kExpressionTextTag = 'Expr',
            kVolumeTag = 'Volm',
            kBitDepthTag = 'BitD'
        };
        
        //------------------------------------------------------------------------
        EvaluatorEditor::EvaluatorEditor (void* controller)
        : VSTGUIEditor (controller)
        , timeLabel(0)
        , millisLabel(0)
        , rangeLabel(0)
        , noteLabel(0)
        , textEdit (0)
        , textResult(0)
        , volumeKnob(0)
        , bitDepthLabel(0)
        , bitDepthSlider(0)
        {
            setIdleRate (50); // 1000ms/50ms = 20Hz
            
            // set the default View size
            ViewRect viewRect (0, 0, kEditorWidth, kEditorHeight);
            setRect (viewRect);
        }
        
        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorEditor::onSize (ViewRect* newSize)
        {
            tresult res = VSTGUIEditor::onSize (newSize);
            return res;
        }
        
        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorEditor::checkSizeConstraint (ViewRect* rect)
        {
            if (rect->right - rect->left < kEditorWidth)
            {
                rect->right = rect->left + kEditorWidth;
            }
            else if (rect->right - rect->left > kEditorWidth+50)
            {
                rect->right = rect->left + kEditorWidth+50;
            }
            if (rect->bottom - rect->top < kEditorHeight)
            {
                rect->bottom = rect->top + kEditorHeight;
            }
            else if (rect->bottom - rect->top > kEditorHeight+50)
            {
                rect->bottom = rect->top + kEditorHeight+50;
            }
            return kResultTrue;
        }
        
        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorEditor::findParameter (int32 xPos, int32 yPos, ParamID& resultId)
        {
            // look up the parameter (view) which is located at xPos/yPos.
            
            CPoint where (xPos, yPos);
            
            // Implementation 1:
            // The parameter xPos/yPos are relative coordinates to the AGainEditorView coordinates.
            // If the window of the VST 3 plugin is moved, xPos is always >= 0 and <= AGainEditorView width.
            // yPos is always >= 0 and <= AGainEditorView height.
            //
            // gainSlider->hitTest() is a short cut for:
            // CRect sliderRect = gainSlider->getMouseableArea ();
            // if (where.isInside (sliderRect))
            // {
            //      resultTag = kGainId;
            //      return kResultOk;
            // }
            
            // test wether xPos/yPos is inside the gainSlider.
            //if (gainSlider->hitTest (where, 0))
            //{
                // return the VST 3 parameter ID, which is also used for IParamValueQueue::getParameterId(),
                // IComponentHandler::performEdit() or IEditController::getParamStringByValue() etc.
            //    resultTag = kGainId;
            //    return kResultOk;
            //}
            
            // test wether xPos/yPos is inside the gain text view.
            //if (gainTextEdit->hitTest (where, 0))
            //{
            //    resultTag = kGainId;
            //    return kResultOk;
            //}
            
            // Implementation 2:
            // An alternative solution with VSTGui can look like this. (This requires C++ RTTI)
            //
            if (frame)
            {
                auto viewOptions = GetViewOptions(GetViewOptions::kMouseEnabled);
                CControl* controlAtPos = dynamic_cast<CControl*>( frame->getViewAt (where,viewOptions) );
                if (controlAtPos)
                {
                    switch (controlAtPos->getTag ())
                    {
                        case kVolumeTag:
                        {
                            resultId = kVolumeId;
                        }
                        return kResultOk;
                    }
                }
            }
            
            
            // Implementation 3:
            // The another "dirty" way is to hard code the coordinates for the views (see also AGainEditorView::open):
            // CRect gainSliderSize (0, 0, 130, 18);
            // gainSliderSize.offset (45, 40);
            // if (where.isInside (gainSliderSize))
            //  {
            //      resultTag = kGainId;
            //      return kResultOk;
            //  }
            // CRect gainTextSize (0, 0, 40, 18);
            // gainTextSize.offset (50 + gainSliderSize.getWidth (), 40);
            // if (where.isInside (gainTextSize))
            //  {
            //      resultTag = kGainId;
            //      return kResultOk;
            //  }
            return kResultFalse;
        }
        
        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorEditor::queryInterface (const char* iid, void** obj)
        {
            QUERY_INTERFACE (iid, obj, IParameterFinder::iid, IParameterFinder)
            QUERY_INTERFACE (iid, obj, IContextMenuTarget::iid, IContextMenuTarget)
            return VSTGUIEditor::queryInterface (iid, obj);
        }
        
        //------------------------------------------------------------------------
        bool EvaluatorEditor::open (void* parent, const PlatformType& platformType)
        {
            if (frame) // already attached!
            {
                return false;
            }
            
            CRect editorSize (0, 0, kEditorWidth, kEditorHeight);

			const CColor textColor = MakeCColor(180,180,180);
			const CColor greenColor = MakeCColor(0, 210, 10);
            
            frame = new CFrame (editorSize, this);
            frame->setBackgroundColor (MakeCColor(30,30,30));
            
            background = new CBitmap (BACKGROUND);
            //frame->setBackground (background);
            
            // used by multiple sliders, so we new them up here
            CBitmap* sliderHandle = new CBitmap (SLIDER_KNOB);
            CBitmap* sliderBackground = new CBitmap (SLIDER_BACK);
            
            float layoutY = 10;
            
            //-- labels for expression variables
            CRect size(0, 0, 100, 18);
            size.offset(10, 0);
            timeLabel = new CTextLabel(size, "t=0", 0, kNoFrame);
            timeLabel->setBackColor(kTransparentCColor);
            timeLabel->setFont(kDataFont);
            timeLabel->setFontColor(textColor);
            timeLabel->setHoriAlign(kLeftText);
            frame->addView(timeLabel);
            
            size.offset(size.getWidth()+10, 0);
            millisLabel = new CTextLabel(size, "m=0", 0, kNoFrame);
            millisLabel->setBackColor(kTransparentCColor);
            millisLabel->setFont(kDataFont);
            millisLabel->setFontColor(textColor);
            millisLabel->setHoriAlign(kLeftText);
            frame->addView(millisLabel);
            
            size.offset(size.getWidth()+10, 0);
            rangeLabel = new CTextLabel(size, "r=0", 0, kNoFrame);
            rangeLabel->setBackColor(kTransparentCColor);
            rangeLabel->setFont(kDataFont);
            rangeLabel->setFontColor(textColor);
            rangeLabel->setHoriAlign(kLeftText);
            frame->addView(rangeLabel);
            
            size.offset(size.getWidth()+10, 0);
            noteLabel = new CTextLabel(size, "n=0", 0, kNoFrame);
            noteLabel->setBackColor(kTransparentCColor);
            noteLabel->setFont(kDataFont);
            noteLabel->setFontColor(textColor);
            noteLabel->setHoriAlign(kLeftText);
            frame->addView(noteLabel);
            
            //--- Text input for the expression ------
            size (0, 0, kEditorWidth - 20, 20);
            size.offset (10, layoutY + 10);
            textEdit = new CTextEdit (size, this, kExpressionTextTag, "t*128", 0, k3DOut);
            textEdit->setBackColor(MakeCColor(10,10,10));
            textEdit->setFont(kDataFont);
            textEdit->setFontColor(greenColor);
            frame->addView (textEdit);
            
            size.offset(0, 20);
            textResult = new CTextLabel(size, "", 0, kNoFrame);
            textResult->setHoriAlign(kLeftText);
            textResult->setBackColor(kTransparentCColor);
            textResult->setFont(kLabelFont);
            textResult->setFontColor(textColor);
            frame->addView(textResult);
            
            //---Volume--------------------
            {                
                //---Volume Knob-------
				const int knobSize(35);
				const int knobLeft = kEditorWidth - knobSize - 10;
				const int knobTop = kEditorHeight - knobSize - 20;
                size (knobLeft, knobTop, knobLeft + knobSize, knobTop + knobSize);
				volumeKnob = new CKnob(size, this, kVolumeTag, 0, 0, CPoint(0, 0), CKnob::kCoronaDrawing | CKnob::kCoronaOutline);
				volumeKnob->setCoronaInset(2);
				volumeKnob->setCoronaColor(greenColor);
				volumeKnob->setColorShadowHandle(kBlackCColor);
				volumeKnob->setHandleLineWidth(2);
				volumeKnob->setColorHandle(greenColor);
                frame->addView (volumeKnob);

				//---Volume Label--------
				size(size.getBottomLeft().x - 10, size.getBottomLeft().y-5, size.getBottomRight().x + 10, size.getBottomRight().y + 10);
				CTextLabel* label = new CTextLabel(size, "VOL", 0, kNoFrame);
				label->setBackColor(kTransparentCColor);
				label->setFont(kLabelFont);
				label->setFontColor(textColor);
				label->setHoriAlign(kCenterText);
				frame->addView(label);
            }
            
            //---Bit Depth--------------
            {
                //---Bit Depth Label--------
                size(0,0,55,18);
                size.offset(70, layoutY + 45);
                bitDepthLabel = new CTextLabel(size, "Bit Depth", 0, kNoFrame);
                bitDepthLabel->setBackColor(kTransparentCColor);
                bitDepthLabel->setFont(kLabelFont);
                bitDepthLabel->setFontColor(textColor);
                frame->addView(bitDepthLabel);
                
                //---Bit Depth Slider--------
                size(0,0,25,122);
                size.offset(90, layoutY + 65);
                CPoint offset;
                CPoint offsetHandle(0,4);
                bitDepthSlider = new CVerticalSlider(size, this, kBitDepthTag,
                                                     offsetHandle, size.getHeight(),
                                                     sliderHandle, sliderBackground,
                                                     offset, kBottom);
                frame->addView(bitDepthSlider);
            }
            
            
            // release the references
            sliderHandle->forget ();
            sliderBackground->forget ();
            
            
            //---VuMeter--------------------
//            CBitmap* onBitmap = new CBitmap ("vu_on.bmp");
//            CBitmap* offBitmap = new CBitmap ("vu_off.bmp");
//            
//            size (0, 0, 12, 105);
//            size.offset (290, 10);
//            vuMeter = new CVuMeter (size, onBitmap, offBitmap, 26, kVertical);
//            frame->addView (vuMeter);
//            onBitmap->forget ();
//            offBitmap->forget ();
//            
            // sync UI controls with controller parameter values
            update( kVolumeId, getController ()->getParamNormalized (kVolumeId) );
            update( kBitDepthId, getController()->getParamNormalized(kBitDepthId) );
            
            messageTextChanged ();
            
            frame->open(parent, platformType);
            
            return true;
        }
        
        //------------------------------------------------------------------------
        void EvaluatorEditor::messageTextChanged ()
        {
            EvaluatorController* controller = dynamic_cast<EvaluatorController*> (getController ());
            if (controller)
            {
                const char * text = controller->getDefaultExpression();
                textEdit->setText (text);
                char buffer[128];
                strncpy(buffer, text, 128);
                EXPR_EVAL_ERR err = expression.Compile(buffer);
                if ( err == EEE_NO_ERROR )
                {
                    snprintf(buffer, 128, "Instruction Count: %d", expression.GetInstructionCount());
                    textResult->setText(buffer);
                }
                else
                {
                    char error[256];
                    snprintf(error, 256, "%s at: %s", Expression::ErrorStr(err), expression.GetErrPos());
                    textResult->setText(error);
                }
            }
        }
        
        //------------------------------------------------------------------------
        void PLUGIN_API EvaluatorEditor::close ()
        {
            if (frame)
            {
                frame->close();
                frame = 0;
            }
            
            if (background)
            {
                background->forget ();
                background = 0;
            }
            
            timeLabel = 0;
            millisLabel = 0;
            rangeLabel = 0;
            noteLabel = 0;
            textEdit = 0;
            textResult = 0;
            volumeKnob = 0;
            bitDepthLabel = 0;
            bitDepthSlider = 0;
            //vuMeter = 0;
        }
        
        //------------------------------------------------------------------------
        void EvaluatorEditor::valueChanged (CControl* pControl)
        {
            switch (pControl->getTag ())
            {
                    //------------------
                case kVolumeTag:
                {
                    controller->setParamNormalized (kVolumeId, pControl->getValue ());
                    controller->performEdit (kVolumeId, pControl->getValue ());
                }
                break;
                    
                case kBitDepthTag:
                {
                    controller->setParamNormalized(kBitDepthId, pControl->getValue());
                    controller->performEdit(kBitDepthId, pControl->getValue());
                }
                break;
//
//                    //------------------
//                case 'GaiT':
//                {
//                    char text[128];
//                    gainTextEdit->getText (text);
//                    
//                    String128 string;
//                    String tmp (text);
//                    tmp.copyTo16 (string, 0, 127);
//                    
//                    ParamValue valueNormalized;
//                    controller->getParamValueByString (kGainId, string, valueNormalized);
//                    
//                    gainSlider->setValue ((float)valueNormalized);
//                    valueChanged (gainSlider);
//                    gainSlider->invalid ();
//                }	break;
                    
                case kExpressionTextTag:
                {
                    // FIXME: we hit this code but for some reason the GUI doesn't update
                    const char* text = textEdit->getText();
                    char buffer[128];
                    strncpy(buffer, text, 128);
                    EXPR_EVAL_ERR err = expression.Compile(buffer);
                    if ( err == EEE_NO_ERROR )
                    {
                        snprintf(buffer, 128, "Instruction Count: %d", expression.GetInstructionCount());
                        textResult->setText(buffer);
                    }
                    else
                    {
                        char error[256];
                        snprintf(error, 256, "%s at: %s", Expression::ErrorStr(err), expression.GetErrPos());
                        textResult->setText(error);
                    }
                    
                    EvaluatorController* controller = dynamic_cast<EvaluatorController*> (getController ());
                    if (controller)
                    {                        
                        controller->setDefaultExpression(textEdit->getText());
                        controller->sendTextMessage(textEdit->getText());
                    }
                }
                break;
            }		
        }
        
        //------------------------------------------------------------------------
        tresult EvaluatorEditor::executeMenuItem (int32 tag)
        {
            // our menu item was choosen by the user
//            if (tag == 1234)
//            {
//                ParameterInfo paramInfo;
//                controller->getParameterInfo (kGainId, paramInfo);
//                
//                controller->beginEdit (kGainId);
//                controller->setParamNormalized (kGainId, paramInfo.defaultNormalizedValue);
//                controller->performEdit (kGainId, paramInfo.defaultNormalizedValue);
//                controller->endEdit (kGainId);
//                
//                return kResultTrue;
//            }
            return kResultFalse;
        }
        
        //------------------------------------------------------------------------
        int32_t EvaluatorEditor::controlModifierClicked (CControl* pControl, CButtonState buttonState)
        {
// TODO will we need to add anything to context menus?
//            switch (pControl->getTag ())
//            {
//                    //------------------
//                case 'GaiT':
//                case 'Gain':
//                {
//                    if (button & kRButton)
//                    {
//                        FUnknownPtr<IComponentHandler3> ch3 (controller->getComponentHandler ());
//                        if (ch3)
//                        {
//                            ParamID pid = kGainId;
//                            IContextMenu* menu = ch3->createContextMenu (this, &pid);
//                            if (menu)
//                            {
//                                // here we add our item (optional)
//                                IContextMenu::Item item = {0};
//                                ConstString ("Set Gain to Default").copyTo16 (item.name, 0, 127);
//                                item.tag = 1234;
//                                menu->addItem (item, this);
//                                
//                                // now we could pop-up the menu
//                                CPoint pos;
//                                frame->getCurrentMouseLocation (pos);
//                                menu->popup (pos.x, pos.y);
//                                menu->release ();
//                                
//                                return 1;
//                            }
//                        }
//                    }
//                }	break;
//            }
            return 0;
        }
        
        //------------------------------------------------------------------------
        void EvaluatorEditor::controlBeginEdit (CControl* pControl)
        {
            switch (pControl->getTag ())
            {
                case kVolumeTag:
                {
                    controller->beginEdit (kVolumeId);
                }
                break;
                    
                case kBitDepthTag:
                {
                    controller->beginEdit(kBitDepthId);
                }
                break;
            }
        }
        
        //------------------------------------------------------------------------
        void EvaluatorEditor::controlEndEdit (CControl* pControl)
        {
            switch (pControl->getTag ())
            {
                case kVolumeTag:
                {
                    controller->endEdit (kVolumeId);
                }
                break;
                    
                case kBitDepthTag:
                {
                    controller->endEdit(kBitDepthId);
                }
                break;
            }
        }
        
        //------------------------------------------------------------------------
        void EvaluatorEditor::update (ParamID tag, ParamValue value)
        {
            switch (tag)
            {
                case kVolumeId:
                {
                    if ( volumeKnob )
                    {
                        volumeKnob->setValue ((float)value);

						auto color = volumeKnob->getCoronaColor();
						color.alpha = 55 + 200 * value;
						volumeKnob->setCoronaColor(color);
                    }
                }
                break;
                    
                case kBitDepthId:
                {
                    if ( bitDepthSlider )
                    {
                        bitDepthSlider->setValue((float)value);
                    }
                    
                    if ( bitDepthLabel )
                    {
                        const int32 stepCount = kBitDepthMax-kBitDepthMin;
                        const int32 shift = std::min<int32> (stepCount, (int32)(value * (stepCount + 1))) + kBitDepthMin;
                        char text[128];
                        sprintf(text, "%d Bit", shift);
                        bitDepthLabel->setText(text);
                    }
                }
                break;
                    
                    
                case kEvalTId:
                {
                    if ( timeLabel )
                    {
                        uint64 time = (uint64)(value*kMaxInt64u);
                        char text[128];
                        sprintf(text, "t=%llu",time);
                        timeLabel->setText(text);
                    }
                }
                break;
                    
                case kEvalMId:
                {
                    if ( millisLabel )
                    {
                        uint64 millis = (uint64)(value*kMaxInt64u);
                        char text[128];
                        sprintf(text, "m=%llu",millis);
                        millisLabel->setText(text);
                    }
                }
                break;
                    
                case kEvalRId:
                {
                    if ( rangeLabel )
                    {
                        uint64 range = (uint64)(value*kMaxInt64u);
                        char text[128];
                        sprintf(text, "r=%llu", range);
                        rangeLabel->setText(text);
                    }
                }
                break;
                    
                case kEvalNId:
                {
                    if ( noteLabel )
                    {
                        int32 note = (int32)(value*kMaxInt32)-1;
                        char text[128];
                        sprintf(text, "n=%d", note);
                        noteLabel->setText(text);
                    }
                }
                break;
                    
                    //------------------
                //case kVuPPMId:
                //    lastVuMeterValue = (float)value;
                //    break;
            }
        }
        
        //------------------------------------------------------------------------
        CMessageResult EvaluatorEditor::notify (CBaseObject* sender, const char* message)
        {
//            if (message == CVSTGUITimer::kMsgTimer)
//            {
//                if (vuMeter)
//                {
//                    vuMeter->setValue (1.f - ((lastVuMeterValue - 1.f) * (lastVuMeterValue - 1.f)));
//                    lastVuMeterValue = 0.f;
//                }
//            }
            return VSTGUIEditor::notify (sender, message);
        }
    }
}