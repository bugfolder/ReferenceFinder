/******************************************************************************
File:         RFCanvas.h
Project:      ReferenceFinder 4.x
Purpose:      Header for diagram display window class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFCANVAS_H_
#define _RFCANVAS_H_

#include "RFPrefix.h"

#include "ReferenceFinder.h"

#include "wx/window.h"

/**********
class RFCanvas - diagram display window
**********/
class RFCanvas : public wxWindow, private RefDgmr {
public:
  // Rendering settings
  static int sUnitPixels;   // size of unit square on screen
  static int sTextSize;     // size of text on screen
  static int sTextLeading;  // gap between lines of text on screen
  static int sDgmSpacing;   // spacing between diagrams on screen
  
  // Ctor
  RFCanvas(wxWindow* parent);
  
  // Content setting
  bool HasContent();
  void SetContentNone();
  void SetContentMarks(const wxString& x1text, const wxString& y1text,
    const XYPt& pt, const std::vector<RefMark*>& marks);
  void SetContentLines(const wxString& x1text, const wxString& y1text,
    const wxString& x2text, const wxString& y2text, const XYLine& line, 
    const std::vector<RefLine*>& lines);
  void SetContentStatistics();
  
  // Utility
  void SizeImageAndFrame();

  // Drawing
  void DoDrawStatus(bool calibrate = false);
  template <class R>
    void DoDrawRefs(const typename R::bare_t& ar, std::vector<R*>& vr, 
      bool calibrate = false);
  void DoDrawStatistics(bool calibrate = false);
  void DoDrawPS(std::ostream& os);
  void DoDraw(wxDC& dc);

  // Event handling
  void OnPaint(wxPaintEvent& event);

private:
  enum {
    SHOW_NONE,
    SHOW_MARKS,
    SHOW_LINES,
    SHOW_STATS
  } mShowWhat;
  wxString mX1Text;
  wxString mY1Text;
  wxString mX2Text;
  wxString mY2Text;
  wxString mTargetText;
  XYPt mMark;
  XYLine mLine;
  std::vector<RefMark*> mMarks;
  std::vector<RefLine*> mLines;
  std::vector<wxString> mStats;
  
  wxDC* mDC;              // Current DC to draw on
  wxPoint mDgmOrigin;     // current location of the origin on the DC
  int mImageWidth;        // used to calibrate screen image size
  int mImageHeight;       // ditto
  double mDCScale;        // scaling between screen pixels and DC
  bool mIsPrinting;       // true = we're printing, false = screen dwg
  int mPrintPage;         // current printing page
  std::vector<int> mImageBlocks;  // used for printing
  std::vector<int> mBlockPages;   // ditto
  
  int PixelsToDC(double f);
  int ModelToDC(double f);
  wxPoint ModelToDC(const XYPt& p);
    
  // Overridden functions from ancestor class RefDgmr
  void DrawPt(const XYPt& aPt, PointStyle pstyle);
  void DrawLine(const XYPt& fromPt, const XYPt& toPt, LineStyle lstyle);
  void DrawArc(const XYPt& ctr, double rad, double fromAngle,
    double toAngle, bool ccw, LineStyle lstyle);
  void DrawPoly(const std::vector<XYPt>& poly, PolyStyle pstyle);
  void DrawLabel(const XYPt& aPt, const std::string& aString, LabelStyle lstyle);
  
  void DrawCaption(const std::string& aString);
  
  // RFCanvas - specific stuff
  void SetPointStyle(PointStyle pstyle);
  void SetLineStyle(LineStyle lstyle);
  void SetPolyStyle(PolyStyle pstyle);
  void SetLabelStyle(LabelStyle lstyle);
  
  friend class RFPrintout;

  DECLARE_EVENT_TABLE()
};


#endif // _RFCANVAS_H_
