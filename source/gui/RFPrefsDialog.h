/******************************************************************************
File:         RFPrefsDialog.h
Project:      ReferenceFinder 4.x
Purpose:      Header for Preferences dialog
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-26
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFPREFSDIALOG_H_
#define _RFPREFSDIALOG_H_

#include <limits>
#include <string>

#include "RFPrefix.h"

#include "wx/panel.h"

class RFCanvas;
class wxNotebook;

/**********
struct RFPrefsPanel - base class for preferences panel
**********/
class RFPrefsPanel : public wxPanel {
public:
  RFPrefsPanel(wxWindow* parent);
  
protected:
  void AddTextPair(const wxString& caption, wxTextCtrl*& textCtrl);
  void AddCheckBox(const wxString& caption, wxCheckBox*& checkBox);

  bool CheckBounds(const wxString&caption, wxTextCtrl *textCtrl,
                   double value, double minval, double maxval);  
  bool ReadVariable(const std::string& varName,
                    const wxString& caption, wxTextCtrl* textCtrl, double& value, 
    double minval = std::numeric_limits<double>::min(), 
    double maxval = std::numeric_limits<double>::max());
  bool ReadDouble(const wxString& caption, wxTextCtrl* textCtrl, double& value, 
    double minval = std::numeric_limits<double>::min(), 
    double maxval = std::numeric_limits<double>::max());
  bool ReadInt(const wxString& caption, wxTextCtrl* textCtrl, int& value, 
    int minval = std::numeric_limits<int>::min(), 
    int maxval = std::numeric_limits<int>::max());
  
  void MsgError(const wxString& msg);
};


/**********
struct RFDatabasePrefs - settings that affect the mark/line database
**********/
struct RFDatabasePrefs {
  double mPaperWidth;
  double mPaperHeight;
  wxString mPaperWidthAsText;
  wxString mPaperHeightAsText;
  int mMaxRank;
  int mMaxLines;
  int mMaxMarks;
  bool mAxiom1;
  bool mAxiom2;
  bool mAxiom3;
  bool mAxiom4;
  bool mAxiom5;
  bool mAxiom6;
  bool mAxiom7;
  int mNumX;
  int mNumY;
  int mNumA;
  int mNumD;
  double mGoodEnoughError;
  double mMinAspectRatio;
  double mMinAngleSine;
  int mDatabaseStatusSkip;
  bool mVisibilityMatters;
  bool mLineWorstCaseError;
  
  RFDatabasePrefs();
  void ToConfig();
  void FromConfig();
  void ToApp();
  void FromApp();
};


/**********
class RFDatabasePanel - Preferences panel for database settings
**********/
class RFDatabasePanel : public RFPrefsPanel {
public:
  RFDatabasePanel(wxWindow* parent);
  
  void DoApply();
  void DoMakeDefault();
  void DoRestoreDefault();
  void DoFactoryDefault();

private:
  RFDatabasePrefs mDatabasePrefs;
  wxTextCtrl* mPaperWidth;
  wxTextCtrl* mPaperHeight;
  wxTextCtrl* mMaxRank;
  wxTextCtrl* mMaxLines;
  wxTextCtrl* mMaxMarks;
  wxCheckBox* mAxiom1;
  wxCheckBox* mAxiom2;
  wxCheckBox* mAxiom3;
  wxCheckBox* mAxiom4;
  wxCheckBox* mAxiom5;
  wxCheckBox* mAxiom6;
  wxCheckBox* mAxiom7;
  wxTextCtrl* mNumX;
  wxTextCtrl* mNumY;
  wxTextCtrl* mNumA;
  wxTextCtrl* mNumD;
  wxTextCtrl* mGoodEnoughError;
  wxTextCtrl* mMinAspectRatio;
  wxTextCtrl* mMinAngleSine;
  wxTextCtrl* mDatabaseStatusSkip;
  wxCheckBox* mVisibilityMatters;
  wxCheckBox* mLineWorstCaseError;
  
  void Fill();
  bool Read();
};


/**********
struct RFDisplayPrefs - settings that affect the displayed image
**********/
struct RFDisplayPrefs {
  bool mClarifyVerbalAmbiguities;
  bool mAxiomsInVerbalDirections;
  int mUnitPixels;
  int mTextSize;
  int mTextLeading;
  int mDgmSpacing;
  int mSearchNum;
  int mNumBuckets;
  double mBucketSize;
  int mNumTrials;

  RFDisplayPrefs();
  void ToConfig();
  void FromConfig();
  void ToApp();
  void FromApp();
};


/**********
class RFDisplayPanel - Preferences panel for display settings
**********/
class RFDisplayPanel : public RFPrefsPanel {
public:
  RFDisplayPanel(wxWindow* parent);
  
  void DoApply();
  void DoMakeDefault();
  void DoRestoreDefault();
  void DoFactoryDefault();

private:
  RFDisplayPrefs mDisplayPrefs;
  wxTextCtrl* mSearchNum;
  wxCheckBox* mClarifyVerbalAmbiguities;
  wxCheckBox* mAxiomsInVerbalDirections;
  wxTextCtrl* mUnitPixels;
  wxTextCtrl* mTextSize;
  wxTextCtrl* mTextLeading;
  wxTextCtrl* mDgmSpacing;
  wxTextCtrl* mNumBuckets;
  wxTextCtrl* mBucketSize;
  wxTextCtrl* mNumTrials;
  
  void Fill();
  bool Read();
};


/**********
class RFPrefsDialog - Preferences dialog
**********/
class RFPrefsDialog : public wxDialog {
public:
  RFPrefsDialog();
  
  void OnApply(wxCommandEvent& event);
  void OnMakeDefault(wxCommandEvent& event);
  void OnRestoreDefault(wxCommandEvent& event);
  void OnFactoryDefault(wxCommandEvent& event);
private:
  wxNotebook* mNotebook;
  RFDatabasePanel* mDatabasePanel;
  RFDisplayPanel* mDisplayPanel;
  
  DECLARE_EVENT_TABLE()
};


#endif // _RFPREFSDIALOG_H_
