/******************************************************************************
File:         RFFrame.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for main frame class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFFrame.h"
#include "RFThread.h"
#include "RFApp.h"
#include "RFCanvas.h"
#include "RFPrintout.h"
#include "RFStackSizer.h"
#include "parser.h"

#include "wx/statline.h"

#include <sstream>

using namespace std;


/*********
class RFTextCtrl - tweaked wxTextCtrl that emits wxEVT_COMMAND_TEXT_ENTER if it
sees the Enter key. We use this so that both RETURN and ENTER are accepted.
**********/

/*****
Intercept the ENTER key and spit out a wxEVT_COMMAND_TEXT_ENTER event.
*****/
void RFTextCtrl::OnChar(wxKeyEvent& event)
{
  int key = event.GetKeyCode();
  if (key == WXK_NUMPAD_ENTER) {
    wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, GetId());
    event.SetEventObject(this);
    event.SetString(GetValue());
    if (GetEventHandler()->ProcessEvent(event)) return;
  }
  event.Skip();
}

/*****
RFTextCtrl event table
*****/
BEGIN_EVENT_TABLE(RFTextCtrl, wxTextCtrl)
  EVT_CHAR(RFTextCtrl::OnChar)
END_EVENT_TABLE()


/*****
Local object
*****/
Parser gParser;


// the application icon (under Windows, Mac, and OS/2 it is in resource file)
#ifdef __WXGTK__     
  #include "icon.xpm" /* ICON_FRAME */
#endif // __WXGTK__


/**********
class RFFrame - main window for ReferenceFinder
**********/

/*****
RFFrame static member initialization
*****/
int RFFrame::sSearchNum = 5;


/*****
Constructor
*****/
RFFrame::RFFrame(const wxString& title) : 
  wxFrame(NULL, wxID_ANY, title)
{
#ifndef __WXMAC__
  SetIcon(wxICON(ICON_FRAME));
#endif // __WXMAC__
  SetMenuBar(gApp->MakeMenuBar());
  {
    RFStackSizer ss(new wxBoxSizer(wxVERTICAL), this);
    wxPanel* panel = new wxPanel(this);
#ifdef __WXMAC__
    panel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif // __WXMAC__
    {
      RFStackSizer ss(new wxBoxSizer(wxHORIZONTAL), panel);
      {
        const int EXTRA_SPACE = 1;
        RFStackSizer ss(new wxGridSizer(1, 5, 5), 
          wxSizerFlags().Expand().Border(wxALL, 2));
        ss.Add(mPointButton = new wxRadioButton(panel, RFID_POINT, wxT("Point")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
        ss.Add(mLineButton = new wxRadioButton(panel, RFID_LINE, wxT("Line")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
      }
      ss.AddSpacer(5);
      ss.Add(new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxLI_VERTICAL), wxSizerFlags().Expand().Border(wxALL, 2));
      ss.AddSpacer(5);
      {
        const int EXTRA_SPACE = 2;
        RFStackSizer ss(new wxFlexGridSizer(5, 5, 5), 
          wxSizerFlags().Border(wxALL, 2));
        ss.Add(mX1StatText = new wxStaticText(panel, wxID_ANY, wxT("x1 =")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
        ss.Add(mX1TextCtrl = new RFTextCtrl(panel, wxID_ANY, wxEmptyString, 
          wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER));
        ss.AddSpacer(5);
        ss.Add(mY1StatText = new wxStaticText(panel, wxID_ANY, wxT("y1 =")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
        ss.Add(mY1TextCtrl = new RFTextCtrl(panel, wxID_ANY, wxEmptyString, 
          wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER));
        ss.Add(mX2StatText = new wxStaticText(panel, wxID_ANY, wxT("x2 =")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
        ss.Add(mX2TextCtrl = new RFTextCtrl(panel, wxID_ANY, wxEmptyString, 
          wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER));
        ss.AddSpacer(5);
        ss.Add(mY2StatText = new wxStaticText(panel, wxID_ANY, wxT("y2 =")),
          wxSizerFlags().Border(wxTOP, EXTRA_SPACE));
        ss.Add(mY2TextCtrl = new RFTextCtrl(panel, wxID_ANY, wxEmptyString, 
          wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER));
     }
     ss.AddSpacer(10);
    }
    ss.Add(panel, wxSizerFlags().Border(wxALL, 5));
    ss.Add(new wxStaticLine(this), wxSizerFlags().Expand());
    mScrolledWindow = new wxScrolledWindow(this);
    mScrolledWindow->SetScrollRate(10, 10);
    mScrolledWindow->SetBackgroundColour(*wxWHITE);
    {
      RFStackSizer ss(new wxBoxSizer(wxVERTICAL), mScrolledWindow);
      ss.Add(gCanvas = new RFCanvas(mScrolledWindow), 
        wxSizerFlags().Border(wxALL, SCREEN_BORDER));
    }
    ss.Add(mScrolledWindow, wxSizerFlags(1).Expand());
    CreateStatusBar();
  }
  
  // Compute the difference between the frame size and the space available for
  // displaying the canvas image. This is used to set the VirtualSizeHints for
  // the frame after a change in canvas size.
  int fw, fh, sw, sh;
  GetSize(&fw, &fh);
  mScrolledWindow->GetSize(&sw, &sh);
  mWidthDiff = fw - sw + 2 * SCREEN_BORDER;
  mHeightDiff = fh - sh + 2 * SCREEN_BORDER;
  
  // Also record the minimum size of the frame (for zero-size canvas) and the
  // corresponding visible image width.
  mMinSize = GetBestSize();
  mMinImageWidth = mMinSize.x - mWidthDiff;
  
  DoSetPoints();
}


/*****
Validate one of the text fields for a valid, parseable expression that lies
within the specified range. Return true if it's valid and set ret to the
numerical value.
*****/
bool RFFrame::ValidateEntry(wxTextCtrl* textCtrl, double minVal, double maxVal, 
  double& ret)
{
  string buffer = textCtrl->GetValue().c_str();
  Parser::Status status = gParser.evaluate(buffer, ret, false);
  if (! status.isOK ()) {
    wxString msg = wxString::Format(wxT("\"%s\" cannot be parsed: %s"),
      buffer.c_str(), status.toString().c_str());
    wxMessageBox(msg, wxT("Parser Error"), wxOK | wxICON_ERROR);
    return false;
  }
  if (ret < minVal || ret > maxVal) {
    wxString msg = wxString::Format(
      wxT("\"%s\" evaluates to %g, but this value should be between %g and %g"),
      buffer.c_str(), ret, minVal, maxVal);
    wxMessageBox(msg, wxT("Range Error"), wxOK | wxICON_ERROR);
    return false;
  }
  return true;
}


/*****
Set controls to search for marks
*****/
void RFFrame::DoSetPoints()
{
  mPointButton->SetValue(true);
  mX2StatText->Disable();
  mX2TextCtrl->Clear();
  mX2TextCtrl->Disable();
  mY2StatText->Disable();
  mY2TextCtrl->Clear();
  mY2TextCtrl->Disable();

  mX1TextCtrl->SetSelection(-1, -1);
  mX1TextCtrl->SetFocus();
}


/*****
Set controls to search for lines
*****/
void RFFrame::DoSetLines()
{
  mLineButton->SetValue(true);
  mX2StatText->Enable();
  mX2TextCtrl->Enable();
  mY2StatText->Enable();
  mY2TextCtrl->Enable();

  mX2TextCtrl->SetSelection(-1, -1);
  mX2TextCtrl->SetFocus();
}


/*****
Update the frame size settings based on a new canvas size
*****/
void RFFrame::SizeFromCanvas(int imageWidth, int imageHeight)
{
  int newWidth = max_val(mMinSize.x, mWidthDiff + imageWidth);
  int newHeight = max_val(mMinSize.y, mHeightDiff + imageHeight);

#ifdef __WXGTK__
  /* In Unix-like displays, the amount of vertical space taken by
     panels, task bars and title bars depends on the particular
     window/desktop manager.  Unfortunately, wxGTK's
     wxGetClientDisplayRect doesn't handle that space, so we reserve
     additional, arbitrary height.
  */
#  define RESERVED_HEIGHT 75
#else
#  define RESERVED_HEIGHT 0
#endif

  // Compute the new size of the frame; show all of the image if possible, but
  // no larger than the available screen area. If the expanded image is too
  // large to fit the display, then add extra space to the frame to accommodate
  // scroll bars.
  const int SCROLLBAR_SIZE = 15;
  wxRect displayRect = wxGetClientDisplayRect();
  if (newWidth > displayRect.width) {
    newWidth = displayRect.width;
    newHeight += SCROLLBAR_SIZE;
  }
  if (newHeight > displayRect.height - RESERVED_HEIGHT) {
    newHeight = displayRect.height - RESERVED_HEIGHT;
    newWidth += SCROLLBAR_SIZE;
  }
  SetSizeHints(mMinSize.x, mMinSize.y, newWidth, newHeight);
  
  // Reposition the window if necessary to keep it from going off the screen.
  int newX, newY;
  GetPosition(&newX, &newY);
  newX = min_val(newX, displayRect.GetRight() - newWidth);
  newX = max_val(newX, displayRect.GetLeft());
  newY = min_val(newY, displayRect.GetBottom() - RESERVED_HEIGHT - newHeight);
  newY = max_val(newY, displayRect.GetTop());

  // Finally, we can resize and reposition the window. Also tell the scrolled
  // window to reset the scrollbars for the new size image.
  SetSize(newX, newY, newWidth, newHeight);
  mScrolledWindow->SetScrollbars(10, 10, 0, 0);
  mScrolledWindow->FitInside();
  Refresh();
}


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Handle Export Postscript menu item
*****/
void RFFrame::OnExportPS(wxCommandEvent&)
{
  if (!gCanvas || !gCanvas->HasContent()) return;
  
  wxFileDialog fileDialog(this, wxT("Export PostScript"), wxEmptyString, 
    wxT("untitled.ps"), wxT("*.ps"), wxSAVE | wxOVERWRITE_PROMPT);
  if (fileDialog.ShowModal() == wxID_CANCEL) return;
  wxString fileName = fileDialog.GetPath();
  
  ofstream fout(fileName.c_str());
  if (!fout.is_open()) {
    wxString msg = wxString::Format(wxT("Sorry, unable to open file \"%s\""),
      fileName.c_str());
    wxMessageBox(msg, wxT("Export Error"), wxICON_ERROR | wxOK, this);
    return;
  }
  
  gCanvas->DoDrawPS(fout);
  fout.close();
}


/*****
Update Export Postscript menu item
*****/
void RFFrame::OnExportPSUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(gCanvas && gCanvas->HasContent());
}


/*****
Handle the Print command
*****/
void RFFrame::OnPrint(wxCommandEvent&)
{
  wxPrintDialogData printDialogData(*gApp->GetPrintData());
  wxPrinter printer(&printDialogData);
  RFPrintout printout;
  if (!printer.Print(this, &printout, true)) {
    if (wxPrinter::GetLastError() == wxPRINTER_ERROR) {
      wxString msg = wxT(\
        "There was a problem printing.\n"\
        "Perhaps your current printer is not set correctly?");
      wxMessageBox(msg, wxT("Printing Error"), wxOK | wxICON_ERROR);
    }
  } 
  else
    (*gApp->GetPrintData()) = printer.GetPrintDialogData().GetPrintData();
}


/*****
Update the Print command
*****/
void RFFrame::OnPrintUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(gCanvas && gCanvas->HasContent());
}


/*****
Handle the Print Preview command
*****/
void RFFrame::OnPrintPreview(wxCommandEvent&)
{
  wxPrintDialogData printDialogData(*gApp->GetPrintData());
  wxPrintPreview *preview = new wxPrintPreview(new RFPrintout, new RFPrintout, 
    &printDialogData);
  if (!preview->Ok())
  {
    delete preview;
    wxString msg = wxT(\
      "There was a problem previewing.\n"\
      "Perhaps your current printer is not set correctly?");
    wxMessageBox(msg, wxT("Previewing Error"), wxOK | wxICON_ERROR);
    return;
  }
  wxPreviewFrame* frame = new wxPreviewFrame(preview, this, 
    wxT("ReferenceFinder Print Preview"), wxPoint(100, 100), wxSize(600, 650));
  frame->Center(wxBOTH);
  frame->Initialize();
  frame->Show();
}


/*****
Update the Print Preview command
*****/
void RFFrame::OnPrintPreviewUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(gCanvas && gCanvas->HasContent());
}


/*****
Handle the Marks command
*****/
void RFFrame::OnToggleMarksLines(wxCommandEvent&)
{
  if (mPointButton->GetValue())
    DoSetLines();
  else
    DoSetPoints();
}


/*****
Update the Marks command
*****/
void RFFrame::OnToggleMarksLinesUpdateUI(wxUpdateUIEvent& event)
{
  if (mPointButton->GetValue()) {
    event.SetText(wxT("Seek Lines\tCtrl+T"));
  } else {
    event.SetText(wxT("Seek Marks\tCtrl+T"));
  }
}


/*****
Handle Get References menu item and button.
*****/
void RFFrame::OnGetReferences(wxCommandEvent&)
{
  // On some platforms, GUI passes command-key equivalents even if menu item
  // isn't enabled, so we silently ignore commands if we're not ready yet.
  if (!RFThread::IsReady()) return;
  
  XYPt p1;
  if (!ValidateEntry(mX1TextCtrl, 0, 
    ReferenceFinder::sPaper.mWidth, p1.x)) return;
  if (!ValidateEntry(mY1TextCtrl, 0, 
    ReferenceFinder::sPaper.mHeight, p1.y)) return;

  if (mPointButton->GetValue()) {
    // Get references for a point
    vector<RefMark*> marks;
    ReferenceFinder::FindBestMarks(p1, marks, sSearchNum);
    if (marks.empty()) {
      wxString msg(wxT("Sorry, ReferenceFinder was unable to find a match for this mark."));
      wxMessageBox(msg, wxT("Search Error"), 
        wxOK | wxICON_ERROR, this);
      return;
    }
    gCanvas->SetContentMarks(mX1TextCtrl->GetValue(), mY1TextCtrl->GetValue(),
      p1, marks);
    return;
  }
  else {
    // Get references for a line
    XYPt p2;
    if (!ValidateEntry(mX2TextCtrl, 0, 
      ReferenceFinder::sPaper.mWidth, p2.x)) return;
    if (!ValidateEntry(mY2TextCtrl, 0, 
      ReferenceFinder::sPaper.mHeight, p2.y)) return;
    if ((p1 - p2).Mag() < 0.01) {
      wxString msg(wxT("Sorry, your points are too close together. Please pick points separated by more than 0.01."));
      wxMessageBox(msg, wxT("Line Error"), 
        wxOK | wxICON_ERROR, this);
      return;
    }
    XYLine line(p1, p2);
    vector<RefLine*> lines;
    ReferenceFinder::FindBestLines(line, lines, sSearchNum);
    if (lines.empty()) {
      wxString msg(wxT("Sorry, ReferenceFinder was unable to find a match for this line."));
      wxMessageBox(msg, wxT("Search Error"), 
        wxOK | wxICON_ERROR, this);
      return;
    }
    gCanvas->SetContentLines(mX1TextCtrl->GetValue(), mY1TextCtrl->GetValue(),
      mX2TextCtrl->GetValue(), mY2TextCtrl->GetValue(), line, lines);
    return;
  }
}


/*****
Update Get References menu item and button
*****/
void RFFrame::OnGetReferencesUpdateUI(wxUpdateUIEvent& event)
{
  bool canGetRefs = RFThread::IsReady();
  event.Enable(canGetRefs);
}


/*****
Handle Clear References command
*****/
void RFFrame::OnClearReferences(wxCommandEvent&)
{
  gCanvas->SetContentNone();
}


/*****
Update the Clear References command
*****/
void RFFrame::OnClearReferencesUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(gCanvas && gCanvas->HasContent());
}


/*****
Handle Rebuild Database menu item
*****/
void RFFrame::OnRebuildDatabase(wxCommandEvent&)
{
  gCanvas->SetContentNone();
  RFDatabaseThread::DoStartDatabase();
}


/*****
Update Rebuild Database menu item
*****/
void RFFrame::OnRebuildDatabaseUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(!RFThread::IsWorking());
}


/*****
Handle Halt Calculation menu item
*****/
void RFFrame::OnHaltCalculation(wxCommandEvent&)
{
  if (RFThread::IsWorking()) RFThread::SetHaltFlag(true);
}


/*****
Update Halt Rebuild menu item
*****/
void RFFrame::OnHaltCalculationUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(RFThread::IsWorking());
  if (RFDatabaseThread::IsWorking()) {
    event.SetText(wxT("Halt Rebuild Database\tCtrl+."));
  }
  else if (RFStatisticsThread::IsWorking()) {
    event.SetText(wxT("Halt Calculate Statistics\tCtrl+."));
  }
  else
    event.SetText(wxT("Halt Calculation\tCtrl+."));
}


/*****
Calculate statistics on the current database
*****/
void RFFrame::OnCalcStatistics(wxCommandEvent&)
{
  if (!RFThread::IsReady()) return;
  RFStatisticsThread::DoStartStatistics();
  gCanvas->SetContentStatistics();
}


/*****
Update the Calc Statistics menu item
*****/
void RFFrame::OnCalcStatisticsUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(RFThread::IsReady());
}


/*****
Handle button press
*****/
void RFFrame::OnRadioButton(wxCommandEvent& event)
{
  event.Skip();
  switch(event.GetId()) {
    case RFID_POINT: {
      DoSetPoints();
      break;
    }
    case RFID_LINE: {
      DoSetLines();
      break;
    }
    default:
      RFFAIL("bad case");
  }
}


/*****
Event tables
*****/
BEGIN_EVENT_TABLE(RFFrame, wxFrame)
  EVT_MENU(RFID_EXPORT_PS, RFFrame::OnExportPS)
  EVT_UPDATE_UI(RFID_EXPORT_PS, RFFrame::OnExportPSUpdateUI)
  EVT_MENU(wxID_PRINT, RFFrame::OnPrint)
  EVT_UPDATE_UI(wxID_PRINT, RFFrame::OnPrintUpdateUI)
  EVT_MENU(wxID_PREVIEW, RFFrame::OnPrintPreview)
  EVT_UPDATE_UI(wxID_PREVIEW, RFFrame::OnPrintPreviewUpdateUI)
  EVT_MENU(RFID_TOGGLE_MARKS_LINES, RFFrame::OnToggleMarksLines)
  EVT_UPDATE_UI(RFID_TOGGLE_MARKS_LINES, RFFrame::OnToggleMarksLinesUpdateUI)
  EVT_MENU(RFID_GET_REFERENCES, RFFrame::OnGetReferences)
  EVT_UPDATE_UI(RFID_GET_REFERENCES, RFFrame::OnGetReferencesUpdateUI)
  EVT_MENU(RFID_CLEAR_REFERENCES, RFFrame::OnClearReferences)
  EVT_UPDATE_UI(RFID_CLEAR_REFERENCES, RFFrame::OnClearReferencesUpdateUI)
  EVT_MENU(RFID_REBUILD, RFFrame::OnRebuildDatabase)
  EVT_UPDATE_UI(RFID_REBUILD, RFFrame::OnRebuildDatabaseUpdateUI)
  EVT_MENU(RFID_HALT_REBUILD, RFFrame::OnHaltCalculation)
  EVT_UPDATE_UI(RFID_HALT_REBUILD, RFFrame::OnHaltCalculationUpdateUI)
  EVT_MENU(RFID_CALC_STATISTICS, RFFrame::OnCalcStatistics)
  EVT_UPDATE_UI(RFID_CALC_STATISTICS, RFFrame::OnCalcStatisticsUpdateUI)
  EVT_RADIOBUTTON(wxID_ANY, RFFrame::OnRadioButton)
  EVT_TEXT_ENTER(wxID_ANY, RFFrame::OnGetReferences)
END_EVENT_TABLE()

