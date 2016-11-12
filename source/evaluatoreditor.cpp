#include "evaluatoreditor.hpp"
#include "evaluatorcontroller.h"
#include "evaluatorids.h"

#include "base/source/fstring.h"

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
        , textEdit (0)
        , volumeSlider(0)
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
            
            frame = new CFrame (editorSize, this);
            frame->setBackgroundColor (kGreyCColor);
            
            background = new CBitmap ("background.png");
            frame->setBackground (background);
            
            // used by multiple sliders, so we new them up here
            CBitmap* sliderHandle = new CBitmap ("vslider_handle.png");
            CBitmap* sliderBackground = new CBitmap ("vslider_background.png");
            
            float layoutY = 10;
            
            //-- labels for expression variables
            CRect size(0, 0, 100, 18);
            size.offset(10, 0);
            timeLabel = new CTextLabel(size, "t=0", 0, kNoFrame);
            timeLabel->setBackColor(kTransparentCColor);
            timeLabel->setFont(kSystemFont);
            timeLabel->setFontColor(kBlackCColor);
            timeLabel->setHoriAlign(kLeftText);
            frame->addView(timeLabel);
            
            size.offset(size.getWidth()+10, 0);
            millisLabel = new CTextLabel(size, "m=0", 0, kNoFrame);
            millisLabel->setBackColor(kTransparentCColor);
            millisLabel->setFont(kSystemFont);
            millisLabel->setFontColor(kBlackCColor);
            millisLabel->setHoriAlign(kLeftText);
            frame->addView(millisLabel);
            
            size.offset(size.getWidth()+10, 0);
            rangeLabel = new CTextLabel(size, "r=0", 0, kNoFrame);
            rangeLabel->setBackColor(kTransparentCColor);
            rangeLabel->setFont(kSystemFont);
            rangeLabel->setFontColor(kBlackCColor);
            rangeLabel->setHoriAlign(kLeftText);
            frame->addView(rangeLabel);
            
            //--- Text input for the expression ------
            size (0, 0, kEditorWidth - 20, 20);
            size.offset (10, layoutY + 10);
            textEdit = new CTextEdit (size, this, kExpressionTextTag, "t*128", 0, k3DOut);
            textEdit->setBackColor(kWhiteCColor);
            textEdit->setFont(kSystemFont);
            textEdit->setFontColor(kBlackCColor);
            frame->addView (textEdit);
            
            //---Volume--------------------
            {
                //---Volume Label--------
                size (0, 0, 50, 18);
                size.offset (10, layoutY + 40);
                CTextLabel* label = new CTextLabel (size, "Volume", 0, kNoFrame);
                label->setBackColor(kTransparentCColor);
                label->setFont(kSystemFont);
                label->setFontColor(kBlackCColor);
                frame->addView (label);
                
                //---Volume slider-------
                
                size (0, 0, 25, 122);
                size.offset (20, layoutY + 60);
                CPoint offset;
                CPoint offsetHandle (0, 4);
                volumeSlider = new CVerticalSlider (size, this, kVolumeTag,
                                                      offsetHandle, size.getHeight(),
                                                      sliderHandle, sliderBackground,
                                                      offset, kBottom);
                frame->addView (volumeSlider);
            }
            
            //---Bit Depth--------------
            {
                //---Bit Depth Label--------
                size(0,0,55,18);
                size.offset(70, layoutY + 40);
                bitDepthLabel = new CTextLabel(size, "Bit Depth", 0, kNoFrame);
                bitDepthLabel->setBackColor(kTransparentCColor);
                bitDepthLabel->setFont(kSystemFont);
                bitDepthLabel->setFontColor(kBlackCColor);
                frame->addView(bitDepthLabel);
                
                //---Bit Depth Slider--------
                size(0,0,25,122);
                size.offset(90, layoutY + 60);
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
                textEdit->setText (controller->getDefaultExpression());
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
            textEdit = 0;
            volumeSlider = 0;
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
                    if ( volumeSlider )
                    {
                        volumeSlider->setValue ((float)value);
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