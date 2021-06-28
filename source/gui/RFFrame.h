/******************************************************************************
File:         RFFrame.h
Project:      ReferenceFinder 4.x
Purpose:      Header for main frame class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFFRAME_H_
#define _RFFRAME_H_

#include "RFPrefix.h"

#include "wx/frame.h"

class RFCanvas;

/*********
class RFTextCtrl - tweaked wxTextCtrl that emits wxEVT_COMMAND_TEXT_ENTER if it
sees the Enter key. We use this so that both RETURN and ENTER are accepted.
**********/
class RFTextCtrl : public wxTextCtrl {
public:
  RFTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr) :
    wxTextCtrl(parent, id, value, pos, size, style, validator, name) {};
  
  void OnChar(wxKeyEvent& event);
  DECLARE_EVENT_TABLE()
};


/**********
class RFFrame - main window for ReferenceFinder
**********/
class RFFrame : public wxFrame {
public:
  enum {
    SCREEN_BORDER = 10
  };
  static int sSearchNum;

  wxRadioButton* mPointButton;
  wxStaticText* mX1StatText;
  wxTextCtrl* mX1TextCtrl;
  wxStaticText* mY1StatText;
  wxTextCtrl* mY1TextCtrl;
  wxRadioButton* mLineButton;
  wxStaticText* mX2StatText;
  wxTextCtrl* mX2TextCtrl;
  wxStaticText* mY2StatText;
  wxTextCtrl* mY2TextCtrl;
  wxScrolledWindow* mScrolledWindow;
  int mWidthDiff;
  int mHeightDiff;
  wxSize mMinSize;
  int mMinImageWidth;

  RFFrame(const wxString& title);
  
  bool ValidateEntry(wxTextCtrl* textCtrl, double minVal, double maxVal, 
    double& ret);
  void DoSetPoints();
  void DoSetLines();
  void SizeFromCanvas(int imageWidth, int imageHeight);

  void OnExportPS(wxCommandEvent& event);
  void OnExportPSUpdateUI(wxUpdateUIEvent& event);
  void OnPrint(wxCommandEvent& event);
  void OnPrintUpdateUI(wxUpdateUIEvent& event);
  void OnPrintPreview(wxCommandEvent& event);
  void OnPrintPreviewUpdateUI(wxUpdateUIEvent& event);
  void OnToggleMarksLines(wxCommandEvent& event);
  void OnToggleMarksLinesUpdateUI(wxUpdateUIEvent& event);
  void OnGetReferences(wxCommandEvent& event);
  void OnGetReferencesUpdateUI(wxUpdateUIEvent& event);
  void OnClearReferences(wxCommandEvent& event);
  void OnClearReferencesUpdateUI(wxUpdateUIEvent& event);
  void OnRebuildDatabase(wxCommandEvent& event);
  void OnRebuildDatabaseUpdateUI(wxUpdateUIEvent& event);
  void OnHaltCalculation(wxCommandEvent& event);
  void OnHaltCalculationUpdateUI(wxUpdateUIEvent& event);
  void OnCalcStatistics(wxCommandEvent& event);
  void OnCalcStatisticsUpdateUI(wxUpdateUIEvent& event);
  void OnRadioButton(wxCommandEvent& event);

private:
  DECLARE_EVENT_TABLE()
};


#endif // _RFFRAME_H_
