#pragma once
#ifndef NDEBUG

#include "IControl.h"

class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(IGEditorDelegate& dlg, const char* pathToSourceFile, float gridSize)
  : IControl(dlg, IRECT(0, 0, 1, 1))
  , mPathToSourceFile(pathToSourceFile)
  , mGridSize(gridSize)
  {
    mTargetRECT = mRECT;
  }
  
  ~IGraphicsLiveEdit() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int c = GetUI()->GetMouseControlIdx(x, y, true);
    
    if (c > 0)
    {
      IControl* pControl = GetUI()->GetControl(c);
      mMouseDownRECT = pControl->GetRECT();
      mMouseDownTargetRECT = pControl->GetTargetRECT();
      mClickedOnControl = c;
      
      if(GetHandleRect(mMouseDownRECT).Contains(x, y))
      {
        mMouseClickedOnResizeHandle = true;
      }
      
      //TODO: add control?
    }
    else if(mod.R)
    {
//      IPopupMenu menu;
//      menu.AddItem("IBitmapControl");
//      menu.AddItem("IVKnobControl");
//      
//      GetUI()->CreatePopupMenu(menu, x, y);
    }
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if(mMouseClickedOnResizeHandle)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
      IRECT r = pControl->GetRECT();
      float w = r.R - r.L;
      float h = r.B - r.T;
      
      if(w < 0.f || h < 0.f)
      {
        pControl->SetRECT(mMouseDownRECT);
        pControl->SetTargetRECT(mMouseDownTargetRECT);
      }
    }
    mClickedOnControl = -1;
    mMouseClickedOnResizeHandle = false;
    GetUI()->SetAllControlsDirty();
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    int c = GetUI()->GetMouseControlIdx(x, y, true);
    if (c > 0)
    {
      IRECT cr = GetUI()->GetControl(c)->GetRECT();
      IRECT h = GetHandleRect(cr);
      
      if(h.Contains(x, y))
      {
        GetUI()->SetMouseCursor(ECursor::SIZENWSE);
        return;
      }
      else
        GetUI()->SetMouseCursor(ECursor::HAND);
    }
    else
      GetUI()->SetMouseCursor(ECursor::ARROW);
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    float mouseDownX, mouseDownY;
    GetUI()->GetMouseDownPoint(mouseDownX, mouseDownY);
    
    if(mClickedOnControl > 0)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
      IRECT r = pControl->GetRECT();
      
      if(mMouseClickedOnResizeHandle)
      {
        r.R = SnapToGrid(mMouseDownRECT.R + (x - mouseDownX));
        r.B = SnapToGrid(mMouseDownRECT.B + (y - mouseDownY));
        
        if(r.R < mMouseDownRECT.L +mGridSize) r.R = mMouseDownRECT.L+mGridSize;
        if(r.B < mMouseDownRECT.T +mGridSize) r.B = mMouseDownRECT.T+mGridSize;
      }
      else
      {
        r.L = SnapToGrid(mMouseDownRECT.L + (x - mouseDownX));
        r.T = SnapToGrid(mMouseDownRECT.T + (y - mouseDownY));
        r.R = r.L + mMouseDownRECT.W();
        r.B = r.T + mMouseDownRECT.H();
      }
      
      pControl->SetRECT(r);
      pControl->SetTargetRECT(r);
      
      DBGMSG("%i, %i, %i, %i\n", (int) r.L, (int) r.T, (int) r.R, (int) r.B);
      
      GetUI()->SetAllControlsDirty();
    }
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawGrid(mGridColor, g.GetBounds(), mGridSize, mGridSize, &BLEND_25);
    
    for(int i = 1; i < g.NControls(); i++)
    {
      IControl* pControl = g.GetControl(i);
      IRECT cr = pControl->GetRECT();
      
      
      if(pControl->IsHidden())
        g.DrawDottedRect(COLOR_RED, cr);
      else if(pControl->IsGrayed())
        g.DrawDottedRect(COLOR_GREEN, cr);
      else
        g.DrawDottedRect(COLOR_BLUE, cr);
      
      IRECT h = GetHandleRect(cr);
      g.FillTriangle(mRectColor, h.L, h.B, h.R, h.B, h.R, h.T);
      g.DrawTriangle(COLOR_BLACK, h.L, h.B, h.R, h.B, h.R, h.T);
    }
  }
  
  bool IsDirty() override { return true; }

  inline IRECT GetHandleRect(IRECT& r)
  {
    return IRECT(r.R - RESIZE_HANDLE_SIZE, r.B - RESIZE_HANDLE_SIZE, r.R, r.B);
  }

  inline float SnapToGrid(float input)
  {
    if (mGridSize > 1)
      return (float) std::round(input / (float) mGridSize) * mGridSize;
    else
      return input;
  }

private:
  bool mEditModeActive = false;
  bool mLiveEditingEnabled = false;
  bool mMouseClickedOnResizeHandle = false;
  bool mMouseIsDragging = false;
  WDL_String mPathToSourceFile;
  WDL_String mErrorMessage;

  IColor mGridColor = COLOR_GRAY;
  IColor mRectColor = COLOR_WHITE;
  static const int RESIZE_HANDLE_SIZE = 10;

  IRECT mMouseDownRECT = IRECT(0, 0, 0, 0);
  IRECT mMouseDownTargetRECT = IRECT(0, 0, 0, 0);

  float mGridSize = 10;
  int mClickedOnControl = -1;
};

#endif // !NDEBUG
