/******************************************************************************
File:         RFCanvas.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for diagram display window class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFCanvas.h"
#include "RFThread.h"
#include "RFFrame.h"
#include "RFApp.h"

#include <sstream>
#include <iomanip>

using namespace std;


/* For debugging printing, it's helpful to put the print image on a gray
background so we can distinguish printable from nonprintable area. */
#define USE_GRAY_BACKGROUND 0


/*****
RFCanvas static member initialization
*****/
int RFCanvas::sUnitPixels = 72;
int RFCanvas::sTextSize = 10;
int RFCanvas::sTextLeading = 13;
int RFCanvas::sDgmSpacing = 10;


/*****
Constructor
*****/
RFCanvas::RFCanvas(wxWindow* parent) : 
  wxWindow(parent, wxID_ANY),
  mDC(0),
  mDCScale(1.0),
  mIsPrinting(false)
{
}


/*****
Return true if we're showing something
*****/
bool RFCanvas::HasContent()
{
  return (mShowWhat == SHOW_MARKS && mMarks.size()) || 
    (mShowWhat == SHOW_LINES && mLines.size());
}


/*****
Clear the content, typically because we've started rebuilding the database
*****/
void RFCanvas::SetContentNone()
{
  mShowWhat = SHOW_NONE;
  mMarks.clear();
  mLines.clear();
  SizeImageAndFrame();
}


/*****
Set the content to display one or more marks
*****/
void RFCanvas::SetContentMarks(const wxString& x1text, const wxString& y1text,
  const XYPt& pt, const std::vector<RefMark*>& marks)
{
  mShowWhat = SHOW_MARKS;
  mX1Text= x1text;
  mY1Text = y1text;
  mMark = pt;
  mMarks = marks;
  mLines.clear();
  mStats.clear();
  mTargetText = wxString::Format(wxT("Paper: (%s x %s), "), 
    ReferenceFinder::sPaper.mWidthAsText.c_str(), 
    ReferenceFinder::sPaper.mHeightAsText.c_str());
  mTargetText += wxString::Format(wxT("Target: point (%s, %s)"), 
    x1text.c_str(), y1text.c_str());
  SizeImageAndFrame();
}


/*****
Set the content to display one or more lines
*****/
void RFCanvas::SetContentLines(const wxString& x1text, const wxString& y1text,
  const wxString& x2text, const wxString& y2text, const XYLine& line, 
  const std::vector<RefLine*>& lines)
{
  mShowWhat = SHOW_LINES;
  mX1Text= x1text;
  mY1Text = y1text;
  mX2Text= x2text;
  mY2Text = y2text;
  mLine = line;
  mLines = lines;
  mMarks.clear();
  mStats.clear();
  mTargetText = wxString::Format(wxT("Paper: (%s x %s), "), 
    ReferenceFinder::sPaper.mWidthAsText.c_str(), 
    ReferenceFinder::sPaper.mHeightAsText.c_str());
  mTargetText += wxString::Format(wxT("Target: line through (%s, %s) and (%s, %s)"), 
    x1text.c_str(), y1text.c_str(), x2text.c_str(), y2text.c_str());
  SizeImageAndFrame();
}


/*****
Set the content to display statistics on the current database
*****/
void RFCanvas::SetContentStatistics()
{
  mShowWhat = SHOW_STATS;
  mMarks.clear();
  mLines.clear();
  mStats.clear();
  if (RFStatisticsThread::GetStatisticsInfo().mStatus == 
    ReferenceFinder::STATISTICS_DONE) {
    stringstream ss;
    ss << ReferenceFinder::sStatistics;
    while (!ss.eof()) {
      char buf[256];
      ss.getline(buf, 256);
      mStats.push_back(buf);
    }
  }
  SizeImageAndFrame();
}


/*****
Set the image size; also resize the window and set new bounds on frame size.
*****/
void RFCanvas::SizeImageAndFrame()
{
  // Calibrate the image size by drawing while setting the calibration flag to
  // true (which sets mImageWidth and mImageHeight)
  switch (mShowWhat) {
    case SHOW_NONE: {
      DoDrawStatus(true);
      break;
    }
    case SHOW_MARKS: {
      DoDrawRefs(mMark, mMarks, true);
      break;
    }
    case SHOW_LINES: {
      DoDrawRefs(mLine, mLines, true);
      break;
    }
    case SHOW_STATS: {
      DoDrawStatistics(true);
      break;
    }
    default:
      RFFAIL("bad case");
  }

  // Set the size of our image from the image width and height just calculated.
  SetSizeHints(mImageWidth, mImageHeight, mImageWidth, mImageHeight);
  
  // And change the frame size to try to show as much of the new image as
  // possible.
  gFrame->SizeFromCanvas(mImageWidth, mImageHeight);
}


/*****
Draw the status of the database. If calibrate=true, compute the image size
in screen pixels and set mImageWidth and mImageHeight.
*****/
void RFCanvas::DoDrawStatus(bool calibrate)
{
  if (calibrate) {
    mDC = new wxClientDC(this);
  }
  wxFont theFont;
  theFont.SetFamily(wxSWISS);
  theFont.SetPointSize(12);
  mDC->SetFont(theFont);
  mDC->SetTextForeground(wxColor(127, 127, 127));
  ReferenceFinder::DatabaseInfo info = RFDatabaseThread::GetDatabaseInfo();
  stringstream ss;
  switch (info.mStatus) {
    case ReferenceFinder::DATABASE_EMPTY:
      ss << "Empty...";
     break;
      
    case ReferenceFinder::DATABASE_INITIALIZING:
      ss << "Initializing...";
     break;
      
    case ReferenceFinder::DATABASE_WORKING:
    case ReferenceFinder::DATABASE_RANK_COMPLETE:
      ss << "Initializing rank " << info.mRank << ": " <<
        info.mNumLines << " lines, " <<
        info.mNumMarks << " marks";
      break;
    
    case ReferenceFinder::DATABASE_READY:
      mDC->SetTextForeground(*wxBLACK);
      ss << "Ready: " <<
        info.mNumLines << " lines, " <<
        info.mNumMarks << " marks";
      break;
  }
  wxString text = ss.str().c_str();
  int tw, th;
  mDC->GetTextExtent(text, &tw, &th);
  if (calibrate) {
    // while the text height will be pretty much the same no matter what the
    // text is, the width will vary with the text. We only call this routine
    // once, so we'll use a standard width, which is the same as the minimum
    // width left for the image from the minimum-size frame.
    mImageWidth = gFrame->mMinImageWidth;
    mImageHeight = th;
  }
  else {
    int w, h;
#ifdef __WXGTK__
    // to be honest, I'm not sure why this is necessary, in wxGTK 2.6.3 at 
    // least; otherwise the status window is not resized while the database
    // is calculated - CAF
    SetSizeHints (wxSize (tw, th));
    GetParent () -> Layout ();
#endif
    GetSize(&w, &h);
    mDC->DrawText(text, wxPoint((w - tw) / 2, (h - th) / 2));
  }
  if (calibrate) {
    delete mDC;
    mDC = 0;
  }
}


/* Notes on drawing and printing.
In the usual way, all drawing is done through DoDraw(), whether it is drawing
into the main window, into a print preview window, or onto a wxPrinterDC for
printing. For printing, we scale the image to always fit the available width,
which is the paper width minus the page margins. In order to obtain maximum
resolution in the printed image, we do our scaling within DoDrawRefs(), which,
since we only print marks and/or lines, is the only one of our drawing
subroutines (DoDrawStatus(), DoDrawRefs(), and DoDrawStatistics) that is called
for printing.
*/


/*****
Draw a set of marks or lines, showing distance and rank for each sequence. If
the calibrate flag is set, this is a calibration call and instead of drawing
into the DC, we set mImageWidth and mImageHeight to the required size and
record the height of each block in mImageBlocks. The printout will be paginated
in terms of these blocks.
*****/
template <class R>
void RFCanvas::DoDrawRefs(const typename R::bare_t& ar, vector<R*>& vr, 
  bool calibrate)
{
  RFASSERT(!(mIsPrinting && calibrate));
  
  int lastY = 0;
  if (calibrate) {
    mDC = new wxClientDC(this);
    mImageWidth = 0;
    mImageHeight = 0;
    mImageBlocks.clear();
  }
  
  if (!calibrate) {
    mDC->SetPen(*wxTRANSPARENT_PEN);
#if USE_GRAY_BACKGROUND
    static wxBrush brush(wxColor(230, 230, 230));
    mDC->SetBrush(brush);
#else
    mDC->SetBrush(*wxWHITE_BRUSH);
#endif // USE_GRAY_BACKGROUND
    if (! mIsPrinting) {
      wxCoord w, h;
      mDC->GetSize(&w, &h);
      mDC->DrawRectangle(0, 0, w, h);
    }
  }

  wxFont theFont;
  theFont.SetFamily(wxFONTFAMILY_ROMAN);
  theFont.SetPointSize(PixelsToDC(10));
  mDC->SetFont(theFont);

  mDgmOrigin.x = 0;
  mDgmOrigin.y = 0;
  
  if (!mIsPrinting || mPrintPage == mBlockPages[0]) {
    // Show the point we're searching for.
    if (!calibrate) {
      DrawCaption(mTargetText.c_str());
    }
    mDgmOrigin.y += PixelsToDC(sTextLeading);
    mDgmOrigin.y += PixelsToDC(sDgmSpacing);
    
    if (calibrate) {
      mImageBlocks.push_back(mDgmOrigin.y);
      lastY = mDgmOrigin.y;
    }
  }
  
  // Go through our list and draw all the diagrams in a single row. 
  for (size_t irow = 0; irow < vr.size(); irow++) {
    if (!mIsPrinting || mPrintPage == mBlockPages[irow + 1]) {
      vr[irow]->BuildDiagrams();
      int dgmh = ModelToDC(ReferenceFinder::sPaper.mHeight);
      mDgmOrigin.y += dgmh;
      mDgmOrigin.x = 0;
      for (size_t icol = 0; icol < RefBase::sDgms.size(); icol++) {
        if (!calibrate) {
          RefBase::DrawDiagram(*this, RefBase::sDgms[icol]);
        }
        int dgmw = ModelToDC(ReferenceFinder::sPaper.mWidth);
        mDgmOrigin.x += dgmw + PixelsToDC(sDgmSpacing);
        if (calibrate) {
          mImageWidth = max_val(mImageWidth, mDgmOrigin.x);
        }
      }
      
      // Also put the text description below the diagrams   
      mDgmOrigin.x = 0;
      mDgmOrigin.y += PixelsToDC(sDgmSpacing);
      ostringstream sd;
      vr[irow]->PutDistanceAndRank(sd, ar);
      if (!calibrate) {
        DrawCaption(sd.str());
      }
      mDgmOrigin.y += PixelsToDC(sTextLeading);
      for (size_t i = 0; i < RefBase::sSequence.size(); i++) {
        mDgmOrigin.x = 0;
        ostringstream s;
        if (RefBase::sSequence[i]->PutHowto(s)) {
          s << ".";
          if (calibrate) {
            wxString text = s.str().c_str();
            int tw, th, td;
            mDC->GetTextExtent(text, &tw, &th, &td);
            tw += mDgmOrigin.x;
            mImageWidth = max_val(mImageWidth, tw);
          }
          if (!calibrate) {
            DrawCaption(s.str());
          }
          mDgmOrigin.y += PixelsToDC(sTextLeading);
        }
      }
      mDgmOrigin.y += PixelsToDC(sDgmSpacing);
      if (calibrate) {
        mImageBlocks.push_back(mDgmOrigin.y - lastY);
        lastY = mDgmOrigin.y;
      }
    }
  }
  if (calibrate) {
    delete mDC;
    mDC = 0;
    mImageHeight = mDgmOrigin.y;
  }
  
  if (mIsPrinting) {
    mDCScale = 1.0;
  }
}


/*****
Draw database statistics
*****/
void RFCanvas::DoDrawStatistics(bool calibrate)
{
 if (calibrate) {
    mDC = new wxClientDC(this);
    mImageWidth = 0;
    mImageHeight = 0;
  }

  wxFont theFont;
  theFont.SetFamily(wxFONTFAMILY_TELETYPE);
  theFont.SetPointSize(12);
  mDC->SetFont(theFont);
  mDC->SetTextForeground(wxColor(127, 127, 127));
  int lastY = 0;
  
  ReferenceFinder::StatisticsInfo info = RFThread::GetStatisticsInfo();
  switch (info.mStatus) {
    case ReferenceFinder::STATISTICS_BEGIN: {
      wxString text = wxT("Starting Statistics...");
      int tw, th, td;
      mDC->GetTextExtent(text, &tw, &th, &td);
      if (calibrate) {
        mImageWidth = gFrame->mMinImageWidth;
        mImageHeight = th;
      }
      else {
        int w, h;
        GetSize(&w, &h);
        mDC->DrawText(text, wxPoint((w - tw) / 2, (h - th) / 2));
      }
    }
    case ReferenceFinder::STATISTICS_WORKING: {
      wxString text = wxString::Format(wxT("Trial %d, error = %.4f"), 
        int(info.mIndex), info.mError);
      int tw, th, td;
      mDC->GetTextExtent(text, &tw, &th, &td);
      if (calibrate) {
        mImageWidth = gFrame->mMinImageWidth;
        mImageHeight = th;
      }
      else {
        int w, h;
        GetSize(&w, &h);
        mDC->DrawText(text, wxPoint((w - tw) / 2, (h - th) / 2));
      }
      break;
    }
    case ReferenceFinder::STATISTICS_DONE: {
      theFont.SetPointSize(sTextSize);
      mDC->SetFont(theFont);
      mDC->SetTextForeground(*wxBLACK);
      for (size_t i = 0; i < mStats.size(); ++i) {
        if (calibrate) {
          int tw, th, td;
          mDC->GetTextExtent(mStats[i], &tw, &th, &td);
          mImageWidth = max_val(mImageWidth, tw);
          mImageHeight += sTextLeading;
        }
        else {
          mDC->DrawText(mStats[i], wxPoint(0, lastY));
          lastY += sTextLeading;
        }
      }
      break;
    }
    default:
      RFFAIL("bad case");
  }
  
  if (calibrate) {
    delete mDC;
    mDC = 0;
  }
}


/*****
Draw the diagrams in PostScript to a stream.
*****/
void RFCanvas::DoDrawPS(ostream& os)
{
  PSStreamDgmr psdgmr(os);
  switch (mShowWhat) {
    case SHOW_MARKS: {
      psdgmr.PutMarkList(mMark, mMarks);
      break;
    }
    case SHOW_LINES: {
      psdgmr.PutLineList(mLine, mLines);
      break;
    }
    case SHOW_NONE:
    default:
      // intentionally nothing
    ;
  }
}


/*****
Draw the diagrams or database status
*****/
void RFCanvas::DoDraw(wxDC& dc)
{
  mDC = &dc;
  switch (mShowWhat) {
    case SHOW_NONE: {
      DoDrawStatus();
      break;
    }
    case SHOW_MARKS: {
      DoDrawRefs(mMark, mMarks);
      break;
    }
    case SHOW_LINES: {
      DoDrawRefs(mLine, mLines);
      break;
    }
    case SHOW_STATS: {
      DoDrawStatistics();
      break;
    }
    default:
      RFFAIL("bad case");
  }
}


/*****
Handle a paint event
*****/
void RFCanvas::OnPaint(wxPaintEvent&)
{
  wxPaintDC dc(this);
  DoDraw(dc);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Set the current graphics state to the given PointStyle.
*****/
void RFCanvas::SetPointStyle(PointStyle pstyle)
{
  wxColor theColor;
  switch (pstyle) {
    case POINTSTYLE_NORMAL: {
      theColor = *wxBLACK;
      break;
    }
    case POINTSTYLE_HILITE: {
      theColor = wxColor(127, 63, 63);
      break;
    }
    case POINTSTYLE_ACTION: {
      theColor = wxColor(127, 0, 0);
      break;
    }
  }
  mDC->SetPen(*wxTRANSPARENT_PEN);
  mDC->SetBrush(wxBrush(theColor, wxSOLID));
}


/*****
Set the current graphics state to the given LineStyle
*****/
void RFCanvas::SetLineStyle(LineStyle lstyle)
{
  wxPen thePen;
  switch (lstyle) {
    case LINESTYLE_CREASE: {
      thePen = wxPen(*wxBLACK, PixelsToDC(0.25), wxSOLID);
      break;
    }
    case LINESTYLE_EDGE: {
      thePen = wxPen(*wxBLACK, PixelsToDC(1), wxSOLID);
      break;
    }
    case LINESTYLE_HILITE: {
      thePen = wxPen(wxColor(255, 127, 127), PixelsToDC(2), wxSOLID);
      break;
    }
    case LINESTYLE_VALLEY: {
      const wxDash VALLEY_DASHES[2] = { 4, 3 };
      thePen = wxPen(wxColor(127, 127, 0), PixelsToDC(1), wxUSER_DASH);
      thePen.SetDashes(2, VALLEY_DASHES);
      break;
    }
    case LINESTYLE_MOUNTAIN: {
      const wxDash MOUNTAIN_DASHES[6] = { 3, 3, 0, 3, 0, 3 };
      thePen = wxPen(*wxBLACK, PixelsToDC(1), wxUSER_DASH);
      thePen.SetDashes(6, MOUNTAIN_DASHES);
      break;
    }
    case LINESTYLE_ARROW: {
      thePen = wxPen(wxColor(0, 127, 0), PixelsToDC(1), wxSOLID);
      break;
    }
  }
  mDC->SetPen(thePen);
}


/*****
Set the current graphics state to the given PolyStyle
*****/
void RFCanvas::SetPolyStyle(PolyStyle pstyle)
{
  wxColor penColor, polyColor;
  switch (pstyle) {
    case POLYSTYLE_WHITE: {
      penColor = *wxBLACK;
      polyColor = wxColor(242, 242, 255);
      break;
    }
    case POLYSTYLE_COLORED: {
      penColor = *wxBLACK;
      polyColor = wxColor(0, 0, 127);
      break;
    }
    case POLYSTYLE_ARROW: {
      penColor = wxColor(0, 127, 0);
      polyColor = wxColor(242, 255, 242);
      break;
    }
  }
  mDC->SetPen(wxPen(penColor, PixelsToDC(1), wxSOLID));
  mDC->SetBrush(wxBrush(polyColor, wxSOLID));
}


/*****
Set the current graphics state to the given LabelStyle
*****/
void RFCanvas::SetLabelStyle(LabelStyle lstyle)
{
  wxColor theColor;
  switch (lstyle) {
    case LABELSTYLE_NORMAL: {
      theColor = *wxBLACK;
      break;
    }
    case LABELSTYLE_HILITE: {
      theColor = wxColor(127, 63, 63);
      break;
    }
    case LABELSTYLE_ACTION: {
      theColor = wxColor(127, 0, 0);
      break;
    }
  }
  mDC->SetTextForeground(theColor);
}


/*****
Convert a screen pixel distance to DC coordinates (not necessarily 1-1 for
printer DCs).
*****/
int RFCanvas::PixelsToDC(double f)
{
  return int(0.5 + f * mDCScale);
}


/*****
Convert a model distance to a DC distance
*****/
int RFCanvas::ModelToDC(double f)
{
  return int(0.5 + f * sUnitPixels * mDCScale);
}


/*****
Convert a point from model coordinates to DC coordinates
*****/
wxPoint RFCanvas::ModelToDC(const XYPt& p)
{
  return wxPoint(mDgmOrigin.x + ModelToDC(p.x), 
    mDgmOrigin.y - ModelToDC(p.y));
}


/*****
Draw a point in the indicated style.
*****/
void RFCanvas::DrawPt(const XYPt& aPt, PointStyle pstyle)
{
  SetPointStyle(pstyle);
  mDC->DrawCircle(ModelToDC(aPt), PixelsToDC(2));
}


/*****
Draw a line in the indicated style.
*****/
void RFCanvas::DrawLine(const XYPt& fromPt, const XYPt& toPt, 
  LineStyle lstyle)
{
  SetLineStyle(lstyle);
  wxPoint pts[2] = { ModelToDC(fromPt), ModelToDC(toPt) };
  mDC->DrawLines(2, pts);
}


/*****
Draw a arc in the indicated style.
*****/
void RFCanvas::DrawArc(const XYPt& ctr, double rad, double fromAngle,
  double toAngle, bool ccw, LineStyle lstyle)
{
  SetLineStyle(lstyle);

  // wxWidgets doesn't support circular arcs without the wedge lines. So we'll
  // implement arcs as a 3-point spline curve, which wxW does support.
  wxPoint p1 = ModelToDC(ctr + XYPt(rad * cos(fromAngle), rad * sin(fromAngle)));
  wxPoint p2 = ModelToDC(ctr + XYPt(rad * cos(toAngle), rad * sin(toAngle)));
  const double PI = 3.1415926535;
  const double TWO_PI = 6.283185308;
  double arcAngle = toAngle - fromAngle;
  while (arcAngle < -PI) arcAngle += TWO_PI;
  while (arcAngle > PI) arcAngle -= TWO_PI;
  double halfAngle = 0.5 * arcAngle;
  double midAngle = fromAngle + halfAngle;
  double  rval = rad / cos(halfAngle);
  wxPoint pm = ModelToDC(ctr + XYPt(rval * cos(midAngle), rval * sin(midAngle)));
 mDC->DrawSpline(p1.x, p1.y, pm.x, pm.y, p2.x ,p2.y);
}


/*****
Fill and stroke the given poly in the indicated style.
*****/
void RFCanvas::DrawPoly(const vector<XYPt>& poly, PolyStyle pstyle)
{
  SetPolyStyle(pstyle);
  static vector<wxPoint> pts;
  pts.clear();
  for (size_t i = 0; i < poly.size(); ++i)
    pts.push_back(ModelToDC(poly[i]));
  mDC->DrawPolygon(int(poly.size()), &pts[0]);
}


/*****
Draw a text label with its baseline at the point aPt in the indicated style
*****/
void RFCanvas::DrawLabel(const XYPt& aPt, const string& aString, 
  LabelStyle lstyle)
{
  SetLabelStyle(lstyle);
  wxPoint pt = ModelToDC(aPt);
  wxString text = aString.c_str();
  int fw, fh, fd;
  mDC->GetTextExtent(text, &fw, &fh, &fd);
  mDC->DrawText(text, wxPoint(pt.x, pt.y - (fh - fd)));
}


/*****
Draw a text label with its top left corner at the current diagram origin
*****/
void RFCanvas::DrawCaption(const string& aString)
{
  SetLabelStyle(LABELSTYLE_NORMAL);
  wxPoint pt = ModelToDC(XYPt(0, 0));
  wxString text = aString.c_str();
  mDC->DrawText(text, pt);
}


/*****
Event tables
*****/
BEGIN_EVENT_TABLE(RFCanvas, wxWindow)
  EVT_PAINT(RFCanvas::OnPaint)
END_EVENT_TABLE()

