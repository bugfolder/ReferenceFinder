/******************************************************************************
File:         RFPrefsDialog.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for Preferences dialog
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-26
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFPrefsDialog.h"
#include "RFApp.h"
#include "RFStackSizer.h"
#include "RFThread.h"
#include "RFCanvas.h"
#include "RFFrame.h"
#include "parser.h"

#include "wx/statline.h"
#include "wx/config.h"
#include "wx/notebook.h"

using namespace std;

/**********
struct RFPrefsPanel - base class for preferences panel
**********/

/*****
Constructor
*****/
RFPrefsPanel::RFPrefsPanel(wxWindow* parent) :
  wxPanel(parent)
{
#ifdef __WXMAC__
  SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif // __WXMAC__
}


/*****
Add a pair of entries for a labeled text control
*****/
void RFPrefsPanel::AddTextPair(const wxString& caption, wxTextCtrl*& textCtrl)
{
  RFStackSizer ss(new wxGridSizer(2, 5, 5), 
    wxSizerFlags(1).Expand().Border(wxALL, 2));
  ss.Add(new wxStaticText(this, wxID_ANY, caption),
    wxSizerFlags().Right().Border(wxTOP, 2));
  ss.Add(textCtrl = new wxTextCtrl(this, wxID_ANY),
    wxSizerFlags().Left());
}


/*****
Add a checkbox to the current sizer
*****/
void RFPrefsPanel::AddCheckBox(const wxString& caption, wxCheckBox*& checkBox)
{
  RFStackSizer::AddSpacer(1);
  RFStackSizer::Add(checkBox = new wxCheckBox(this, wxID_ANY, caption));
}


/*****
Check the bounds on an entered variable value and report any errors.
*****/
bool RFPrefsPanel::CheckBounds(const wxString& caption, wxTextCtrl *textCtrl,
                               double value, double minval, double maxval) {
 if (value < minval) {
    MsgError(wxString::Format(wxT("Sorry, %s must be greater than %g."), 
      caption.c_str(), minval));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  if (value > maxval) {
    MsgError(wxString::Format(wxT("Sorry, %s must be less than %g."), 
      caption.c_str(), maxval));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  return true;
}


/*****
Read a double value using the parser and validate, reporting an error if
the string input is invalid.
*****/
bool RFPrefsPanel::ReadVariable(const std::string &varName,
                                const wxString& caption, wxTextCtrl* textCtrl, 
                                double& value, double minval, double maxval)
{
  wxString text = textCtrl->GetValue();
  double newValue;
  Parser parser;
  Parser::Status st = parser.evaluate (varName, newValue);
  if (! st.isOK ()) {
    MsgError(wxString::Format(wxT("Sorry, \"%s\" is invalid: %s"), 
      text.c_str(), st.toString ().c_str()));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  if (CheckBounds (caption, textCtrl, newValue, minval, maxval)) {
    value = newValue;
    return true;
  } else
    return false;
}

/*****
Read a double value from the text control and validate. If successful, return
true and write the value to the passed reference.
*****/
bool RFPrefsPanel::ReadDouble(const wxString& caption, wxTextCtrl* textCtrl, 
  double& value, double minval, double maxval)
{
  wxString text = textCtrl->GetValue();
  double newValue;
  if (!text.ToDouble(&newValue)) {
    MsgError(wxString::Format(wxT("Sorry, \"%s\" is not a valid number."), 
      text.c_str()));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  if (CheckBounds (caption, textCtrl, newValue, minval, maxval)) {
    value = newValue;
    return true;
  } else
    return false;
}


/*****
Read a int value from the text control and validate. If successful, return
true and write the value to the passed reference.
*****/
bool RFPrefsPanel::ReadInt(const wxString& caption, wxTextCtrl* textCtrl, 
  int& value, int minval, int maxval)
{
  wxString text = textCtrl->GetValue();
  long newValue;
  if (!text.ToLong(&newValue)) {
    MsgError(wxString::Format(wxT("Sorry, \"%s\" is not a valid number."), 
      text.c_str()));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  if (newValue < minval) {
    MsgError(wxString::Format(wxT("Sorry, %s must be greater than %d."), 
      text.c_str(), minval));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  if (newValue > maxval) {
    MsgError(wxString::Format(wxT("Sorry, %s must be less than %d."), 
      text.c_str(), maxval));
    textCtrl->SetSelection(-1, -1);
    textCtrl->SetFocus();
    return false;
  }
  value = int(newValue);
  return true;
}


/*****
Put up an error box containing a message.
*****/
void RFPrefsPanel::MsgError(const wxString& msg)
{
  wxMessageBox(msg, wxT("Preferences Error"), wxOK | wxICON_ERROR, this);
}



#ifdef __MWERKS__
#pragma mark -
#endif


/**********
struct RFDatabasePrefs - settings that affect the mark/line database
**********/

/*****
Constructor - default ctor defines "factory defaults"
*****/
RFDatabasePrefs::RFDatabasePrefs() :
  mPaperWidth(1.0),
  mPaperHeight(1.0),
  mPaperWidthAsText(wxT("1.0")),
  mPaperHeightAsText(wxT("1.0")),
  mMaxRank(6),
  mMaxLines(500000),
  mMaxMarks(500000),
  mAxiom1(true),
  mAxiom2(true),
  mAxiom3(true),
  mAxiom4(true),
  mAxiom5(true),
  mAxiom6(true),
  mAxiom7(true),
  mNumX(5000),
  mNumY(5000),
  mNumA(5000),
  mNumD(5000),
  mGoodEnoughError(0.005),
  mMinAspectRatio(0.100),
  mMinAngleSine(0.342),
  mDatabaseStatusSkip(20000),
  mVisibilityMatters(true),
  mLineWorstCaseError(true)
{
}


/*****
wxConfig keys
*****/
const wxString KEY_PAPER_WIDTH = wxT("PaperWidth");
const wxString KEY_PAPER_HEIGHT = wxT("PaperHeight");
const wxString KEY_MAX_RANK = wxT("MaxRank");
const wxString KEY_MAX_LINES = wxT("MaxLines");
const wxString KEY_MAX_MARKS = wxT("MaxMarks");
const wxString KEY_AXIOM_1 = wxT("Axiom1");
const wxString KEY_AXIOM_2 = wxT("Axiom2");
const wxString KEY_AXIOM_3 = wxT("Axiom3");
const wxString KEY_AXIOM_4 = wxT("Axiom4");
const wxString KEY_AXIOM_5 = wxT("Axiom5");
const wxString KEY_AXIOM_6 = wxT("Axiom6");
const wxString KEY_AXIOM_7 = wxT("Axiom7");
const wxString KEY_NUM_X = wxT("NumX");
const wxString KEY_NUM_Y = wxT("NumY");
const wxString KEY_NUM_A = wxT("NumA");
const wxString KEY_NUM_D = wxT("NumD");
const wxString KEY_GOOD_ENOUGH_ERROR = wxT("GoodEnoughError");
const wxString KEY_MIN_ASPECT_RATIO = wxT("MinAspectRatio");
const wxString KEY_MIN_ANGLE_SINE = wxT("MinAngleSine");
const wxString KEY_DATABASE_SKIP = wxT("StatusSkip");
const wxString KEY_VISIBILITY_MATTERS = wxT("VisibilityMatters");
const wxString KEY_LINE_WORST_CASE_ERROR = wxT("LineWorstCaseError");


/*****
Write the settings to the configuration file
*****/
void RFDatabasePrefs::ToConfig()
{
  wxConfig::Get()->Write(KEY_PAPER_WIDTH, mPaperWidthAsText);
  wxConfig::Get()->Write(KEY_PAPER_HEIGHT, mPaperHeightAsText);
  wxConfig::Get()->Write(KEY_MAX_RANK, mMaxRank);
  wxConfig::Get()->Write(KEY_MAX_LINES, mMaxLines);
  wxConfig::Get()->Write(KEY_MAX_MARKS, mMaxMarks);
  wxConfig::Get()->Write(KEY_AXIOM_1, mAxiom1);
  wxConfig::Get()->Write(KEY_AXIOM_2, mAxiom2);
  wxConfig::Get()->Write(KEY_AXIOM_3, mAxiom3);
  wxConfig::Get()->Write(KEY_AXIOM_4, mAxiom4);
  wxConfig::Get()->Write(KEY_AXIOM_5, mAxiom5);
  wxConfig::Get()->Write(KEY_AXIOM_6, mAxiom6);
  wxConfig::Get()->Write(KEY_AXIOM_7, mAxiom7);
  wxConfig::Get()->Write(KEY_NUM_X, mNumX);
  wxConfig::Get()->Write(KEY_NUM_Y, mNumY);
  wxConfig::Get()->Write(KEY_NUM_A, mNumA);
  wxConfig::Get()->Write(KEY_NUM_D, mNumD);
  wxConfig::Get()->Write(KEY_GOOD_ENOUGH_ERROR, mGoodEnoughError);
  wxConfig::Get()->Write(KEY_MIN_ASPECT_RATIO, mMinAspectRatio);
  wxConfig::Get()->Write(KEY_MIN_ANGLE_SINE, mMinAngleSine);
  wxConfig::Get()->Write(KEY_DATABASE_SKIP, mDatabaseStatusSkip);
  wxConfig::Get()->Write(KEY_VISIBILITY_MATTERS, mVisibilityMatters);
  wxConfig::Get()->Write(KEY_LINE_WORST_CASE_ERROR, mLineWorstCaseError);
}


/*****
Read the settings from the configuration file
*****/
void RFDatabasePrefs::FromConfig()
{
  RFDatabasePrefs defaults;
  wxConfig::Get()->Read(KEY_PAPER_WIDTH, &mPaperWidthAsText,
                        defaults.mPaperWidthAsText);
  wxConfig::Get()->Read(KEY_PAPER_HEIGHT, &mPaperHeightAsText,
                        defaults.mPaperHeightAsText);
  Parser parser;
  parser.setVariable ("w", mPaperWidthAsText.c_str ());
  parser.setVariable ("h", mPaperHeightAsText.c_str ());
  parser.evaluate ("w", mPaperWidth); // TODO status
  parser.evaluate ("h", mPaperHeight); // TODO status

  wxConfig::Get()->Read(KEY_MAX_RANK, &mMaxRank, defaults.mMaxRank);
  wxConfig::Get()->Read(KEY_MAX_LINES, &mMaxLines, defaults.mMaxLines);
  wxConfig::Get()->Read(KEY_MAX_MARKS, &mMaxMarks, defaults.mMaxMarks);
  wxConfig::Get()->Read(KEY_AXIOM_1, &mAxiom1, defaults.mAxiom1);
  wxConfig::Get()->Read(KEY_AXIOM_2, &mAxiom2, defaults.mAxiom2);
  wxConfig::Get()->Read(KEY_AXIOM_3, &mAxiom3, defaults.mAxiom3);
  wxConfig::Get()->Read(KEY_AXIOM_4, &mAxiom4, defaults.mAxiom4);
  wxConfig::Get()->Read(KEY_AXIOM_5, &mAxiom5, defaults.mAxiom5);
  wxConfig::Get()->Read(KEY_AXIOM_6, &mAxiom6, defaults.mAxiom6);
  wxConfig::Get()->Read(KEY_AXIOM_7, &mAxiom7, defaults.mAxiom7);
  wxConfig::Get()->Read(KEY_NUM_X, &mNumX, defaults.mNumX);
  wxConfig::Get()->Read(KEY_NUM_Y, &mNumY, defaults.mNumY);
  wxConfig::Get()->Read(KEY_NUM_A, &mNumA, defaults.mNumA);
  wxConfig::Get()->Read(KEY_NUM_D, &mNumD, defaults.mNumD);
  wxConfig::Get()->Read(KEY_GOOD_ENOUGH_ERROR, &mGoodEnoughError, defaults.mGoodEnoughError);
  wxConfig::Get()->Read(KEY_MIN_ASPECT_RATIO, &mMinAspectRatio, defaults.mMinAspectRatio);
  wxConfig::Get()->Read(KEY_MIN_ANGLE_SINE, &mMinAngleSine, defaults.mMinAngleSine);
  wxConfig::Get()->Read(KEY_DATABASE_SKIP, &mDatabaseStatusSkip, defaults.mDatabaseStatusSkip);
  wxConfig::Get()->Read(KEY_VISIBILITY_MATTERS, &mVisibilityMatters, defaults.mVisibilityMatters);
  wxConfig::Get()->Read(KEY_LINE_WORST_CASE_ERROR, &mLineWorstCaseError, defaults.mLineWorstCaseError);
}


/*****
Write the settings to the ReferenceFinder static variables
*****/
void RFDatabasePrefs::ToApp()
{
  ReferenceFinder::sPaper.SetSize(mPaperWidth, mPaperHeight);
  ReferenceFinder::sPaper.mWidthAsText = mPaperWidthAsText;
  ReferenceFinder::sPaper.mHeightAsText = mPaperHeightAsText;
  ReferenceFinder::sMaxRank = mMaxRank;
  ReferenceFinder::sMaxLines = mMaxLines;
  ReferenceFinder::sMaxMarks = mMaxMarks;
  ReferenceFinder::sUseRefLine_C2P_C2P = mAxiom1;
  ReferenceFinder::sUseRefLine_P2P = mAxiom2;
  ReferenceFinder::sUseRefLine_L2L = mAxiom3;
  ReferenceFinder::sUseRefLine_L2L_C2P = mAxiom4;
  ReferenceFinder::sUseRefLine_P2L_C2P = mAxiom5;
  ReferenceFinder::sUseRefLine_P2L_P2L = mAxiom6;
  ReferenceFinder::sUseRefLine_L2L_P2L = mAxiom7;
  ReferenceFinder::sNumX = mNumX;
  ReferenceFinder::sNumY = mNumY;
  ReferenceFinder::sNumA = mNumA;
  ReferenceFinder::sNumD = mNumD;
  ReferenceFinder::sGoodEnoughError = mGoodEnoughError;
  ReferenceFinder::sMinAspectRatio = mMinAspectRatio;
  ReferenceFinder::sMinAngleSine = mMinAngleSine;
  ReferenceFinder::sDatabaseStatusSkip = mDatabaseStatusSkip;
  ReferenceFinder::sVisibilityMatters = mVisibilityMatters;
  ReferenceFinder::sLineWorstCaseError = mLineWorstCaseError;
}


/*****
Read the settings from the ReferenceFinder static variables
*****/
void RFDatabasePrefs::FromApp()
{
  mPaperWidth = ReferenceFinder::sPaper.mWidth;
  mPaperHeight = ReferenceFinder::sPaper.mHeight;
  mPaperWidthAsText = ReferenceFinder::sPaper.mWidthAsText.c_str();
  mPaperHeightAsText = ReferenceFinder::sPaper.mHeightAsText.c_str();
  mMaxRank = ReferenceFinder::sMaxRank;
  mMaxLines = ReferenceFinder::sMaxLines;
  mMaxMarks = ReferenceFinder::sMaxMarks;
  mAxiom1 = ReferenceFinder::sUseRefLine_C2P_C2P;
  mAxiom2 = ReferenceFinder::sUseRefLine_P2P;
  mAxiom3 = ReferenceFinder::sUseRefLine_L2L;
  mAxiom4 = ReferenceFinder::sUseRefLine_L2L_C2P;
  mAxiom5 = ReferenceFinder::sUseRefLine_P2L_C2P;
  mAxiom6 = ReferenceFinder::sUseRefLine_P2L_P2L;
  mAxiom7 = ReferenceFinder::sUseRefLine_L2L_P2L;
  mNumX = ReferenceFinder::sNumX;
  mNumY = ReferenceFinder::sNumY;
  mNumA = ReferenceFinder::sNumA;
  mNumD = ReferenceFinder::sNumD;
  mGoodEnoughError = ReferenceFinder::sGoodEnoughError;
  mMinAspectRatio = ReferenceFinder::sMinAspectRatio;
  mMinAngleSine = ReferenceFinder::sMinAngleSine;
  mDatabaseStatusSkip = ReferenceFinder::sDatabaseStatusSkip;
  mVisibilityMatters = ReferenceFinder::sVisibilityMatters;
  mLineWorstCaseError = ReferenceFinder::sLineWorstCaseError;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFDatabasePanel - Preferences panel for database settings
**********/

/*****
Constructor
*****/
RFDatabasePanel::RFDatabasePanel(wxWindow* parent) :
  RFPrefsPanel(parent)
{
  RFStackSizer ss(new wxBoxSizer(wxVERTICAL), this);
  {
    RFStackSizer ss(new wxGridSizer(2, 2, 2), wxSizerFlags().Expand());
    { // First column
      RFStackSizer ss(new wxBoxSizer(wxVERTICAL));
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this, 
          wxT("Paper")), wxSizerFlags().Expand());
        {
          RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
          AddTextPair(wxT("Paper Width:"), mPaperWidth);
          AddTextPair(wxT("Paper Height:"), mPaperHeight);
        }
      }
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this, 
          wxT("Size Limits")), wxSizerFlags().Expand());
        {
          RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
          AddTextPair(wxT("Max Rank:"), mMaxRank);
          AddTextPair(wxT("Max Lines:"), mMaxLines);
          AddTextPair(wxT("Max Marks:"), mMaxMarks);
        }
      }
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this, 
          wxT("Huzita-Hatori Axioms")), wxSizerFlags().Expand());
        {
          RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
          AddCheckBox(wxT("O1 - Crease Through Two Points"), mAxiom1);
          AddCheckBox(wxT("O2 - Two Points Together"), mAxiom2);
          AddCheckBox(wxT("O3 - Line to Another Line"), mAxiom3);
          AddCheckBox(wxT("O4 - Line to Self, Crease Through Point"), mAxiom4);
          AddCheckBox(wxT("O5 - Point to Line, Crease Through Point"), mAxiom5);
          AddCheckBox(wxT("O6 - Two Points to Two Lines"), mAxiom6);
          AddCheckBox(wxT("O7 - Line to Self, Point to Line"), mAxiom7);
        }
      }
    }
    { // 2nd column
      RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this, 
          wxT("Key Size")), wxSizerFlags().Expand());
        {
          RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
          AddTextPair(wxT("Marks - X Divisions:"), mNumX);
          AddTextPair(wxT("Marks - Y Divisions:"), mNumY);
          AddTextPair(wxT("Lines - Angle Divisions:"), mNumA);
          AddTextPair(wxT("Lines - Radial Divisions:"), mNumD);
        }
      }
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this, 
          wxT("Misc Settings")), wxSizerFlags().Expand());
        {
          RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
          AddTextPair(wxT("\"Good Enough\" Error:"), mGoodEnoughError);
          AddTextPair(wxT("Minimum Aspect Ratio:"), mMinAspectRatio);
          AddTextPair(wxT("Minimum Angle Sine"), mMinAngleSine);
          AddTextPair(wxT("Database Status Skip"), mDatabaseStatusSkip);
          AddCheckBox(wxT("Visibility Matters"), mVisibilityMatters);
          AddCheckBox(wxT("Lines Use Worst-Case Error"), mLineWorstCaseError);
        }
      }
    }
  }
  mDatabasePrefs.FromApp();
  Fill();
}


/*****
Fill the database panel from static variables
*****/
void RFDatabasePanel::Fill()
{
  mPaperWidth->SetValue(mDatabasePrefs.mPaperWidthAsText);
  mPaperWidth->SetSelection(-1, -1);
  mPaperHeight->SetValue(mDatabasePrefs.mPaperHeightAsText);
  mPaperHeight->SetSelection(-1, -1);
  mMaxRank->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mMaxRank));
  mMaxRank->SetSelection(-1, -1);
  mMaxLines->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mMaxLines));
  mMaxLines->SetSelection(-1, -1);
  mMaxMarks->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mMaxMarks));
  mMaxMarks->SetSelection(-1, -1);
  mAxiom1->SetValue(mDatabasePrefs.mAxiom1);
  mAxiom2->SetValue(mDatabasePrefs.mAxiom2);
  mAxiom3->SetValue(mDatabasePrefs.mAxiom3);
  mAxiom4->SetValue(mDatabasePrefs.mAxiom4);
  mAxiom5->SetValue(mDatabasePrefs.mAxiom5);
  mAxiom6->SetValue(mDatabasePrefs.mAxiom6);
  mAxiom7->SetValue(mDatabasePrefs.mAxiom7);
  mNumX->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mNumX));
  mNumX->SetSelection(-1, -1);
  mNumY->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mNumY));
  mNumY->SetSelection(-1, -1);
  mNumA->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mNumA));
  mNumA->SetSelection(-1, -1);
  mNumD->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mNumD));
  mNumD->SetSelection(-1, -1);
  mGoodEnoughError->SetValue(wxString::Format(wxT("%.4f"), mDatabasePrefs.mGoodEnoughError));
  mGoodEnoughError->SetSelection(-1, -1);
  mMinAspectRatio->SetValue(wxString::Format(wxT("%.4f"), mDatabasePrefs.mMinAspectRatio));
  mMinAspectRatio->SetSelection(-1, -1);
  mMinAngleSine->SetValue(wxString::Format(wxT("%.4f"), mDatabasePrefs.mMinAngleSine));
  mMinAngleSine->SetSelection(-1, -1);
  mDatabaseStatusSkip->SetValue(wxString::Format(wxT("%d"), mDatabasePrefs.mDatabaseStatusSkip));
  mDatabaseStatusSkip->SetSelection(-1, -1);
  mVisibilityMatters->SetValue(mDatabasePrefs.mVisibilityMatters);
  mLineWorstCaseError->SetValue(mDatabasePrefs.mLineWorstCaseError);
}


/*****
Read and validate the entries in the database panel, returning true if
successful and transferring them into the static variables
*****/
bool RFDatabasePanel::Read()
{
  /* Logic is a bit convoluted here. We need commiting both w and h to
     the parser dictionary, since one may use the other. However, we
     must restore the old values if any constraint fails. Since there
     are many tests, it's easier restoring then anyway, then commiting
     present values again at the end if all is right.
  */
  Parser::Value oldW = Parser::getVariable ("w"),
    oldH = Parser::getVariable ("h");
  bool ok = true;

  Parser::setVariable ("w", mPaperWidth -> GetValue ().c_str ());
  Parser::setVariable ("h", mPaperHeight -> GetValue ().c_str ());
  if (!ReadVariable("w", wxT("paper width"), mPaperWidth,
                    mDatabasePrefs.mPaperWidth, 0.0)
      || !ReadVariable("h", wxT("paper height"), mPaperHeight,
                       mDatabasePrefs.mPaperHeight, 0.0))
    ok = false;
  Parser::setVariable ("w", oldW);
  Parser::setVariable ("h", oldH);
  if (!ok)
    return false;

  if (!ReadInt(wxT("maximum rank"), mMaxRank, 
    mDatabasePrefs.mMaxRank, 1)) return false;
  if (!ReadInt(wxT("maximum lines"), mMaxLines, 
    mDatabasePrefs.mMaxLines, 1)) return false;
  if (!ReadInt(wxT("maximum marks"), mMaxMarks, 
    mDatabasePrefs.mMaxMarks, 1)) return false;
  mDatabasePrefs.mAxiom1 = mAxiom1->GetValue();
  mDatabasePrefs.mAxiom2 = mAxiom2->GetValue();
  mDatabasePrefs.mAxiom3 = mAxiom3->GetValue();
  mDatabasePrefs.mAxiom4 = mAxiom4->GetValue();
  mDatabasePrefs.mAxiom5 = mAxiom5->GetValue();
  mDatabasePrefs.mAxiom6 = mAxiom6->GetValue();
  mDatabasePrefs.mAxiom7 = mAxiom7->GetValue();
  if (!(mDatabasePrefs.mAxiom1 || mDatabasePrefs.mAxiom2 || 
    mDatabasePrefs.mAxiom3 || mDatabasePrefs.mAxiom4 || 
    mDatabasePrefs.mAxiom5 || mDatabasePrefs.mAxiom6 || 
    mDatabasePrefs.mAxiom7)) {
    MsgError(wxT("Sorry, you must select at least one axiom."));
    return false;
  }
  if (!ReadInt(wxT("X divisions"), mNumX, mDatabasePrefs.mNumX, 2)) return false;
  if (!ReadInt(wxT("Y divisions"), mNumY, mDatabasePrefs.mNumY, 2)) return false;
  if (mDatabasePrefs.mNumX > std::numeric_limits<ReferenceFinder::key_t>::max() / 
    mDatabasePrefs.mNumY) {
    MsgError(wxT("Sorry, you must reduce either the number of X buckets or Y buckets"));
    return false;
  }
  if (!ReadInt(wxT("angular divisions"), mNumA, mDatabasePrefs.mNumA, 2)) return false;
  if (!ReadInt(wxT("radial divisions"), mNumD, mDatabasePrefs.mNumD, 2)) return false;
  if (mDatabasePrefs.mNumA > std::numeric_limits<ReferenceFinder::key_t>::max() / 
    mDatabasePrefs.mNumD) {
    MsgError(wxT("Sorry, you must reduce either the number of A buckets or D buckets"));
    return false;
  }
  if (!ReadDouble(wxT("\"good enough\" error"), mGoodEnoughError, 
    mDatabasePrefs.mGoodEnoughError, 0.0)) return false;
  if (!ReadDouble(wxT("minimum aspect ratio"), mMinAspectRatio, 
    mDatabasePrefs.mMinAspectRatio, 0.0)) return false;
  if (!ReadDouble(wxT("minimum angle sine"), mMinAngleSine, 
    mDatabasePrefs.mMinAngleSine, 0.0, 1.0)) return false;
  if (!ReadInt(wxT("status skip"), mDatabaseStatusSkip, mDatabasePrefs.mDatabaseStatusSkip, 0)) return false;

  mDatabasePrefs.mVisibilityMatters = mVisibilityMatters->GetValue();
  mDatabasePrefs.mLineWorstCaseError = mLineWorstCaseError->GetValue();
  mDatabasePrefs.mPaperWidthAsText = mPaperWidth -> GetValue ();
  mDatabasePrefs.mPaperHeightAsText = mPaperHeight -> GetValue ();
  Parser::setVariable ("w", mDatabasePrefs.mPaperWidthAsText.c_str ());
  Parser::setVariable ("h", mDatabasePrefs.mPaperHeightAsText.c_str ());
  return true;
}


/*****
Handle Apply button
*****/
void RFDatabasePanel::DoApply()
{
  if (Read()) {
    RFStatisticsThread::DoHaltStatistics();
    RFDatabaseThread::DoHaltDatabase();
    gCanvas->SetContentNone();
    mDatabasePrefs.ToApp();
    RFDatabaseThread::DoStartDatabase();
  }
}


/*****
Handle Make Default button
*****/
void RFDatabasePanel::DoMakeDefault()
{
  if (Read()) {
    RFStatisticsThread::DoHaltStatistics();
    RFDatabaseThread::DoHaltDatabase();
    gCanvas->SetContentNone();
    mDatabasePrefs.ToApp();
    mDatabasePrefs.ToConfig();
    RFDatabaseThread::DoStartDatabase();
  }
}


/*****
Handle Restore Default button
*****/
void RFDatabasePanel::DoRestoreDefault()
{
  RFStatisticsThread::DoHaltStatistics();
  RFDatabaseThread::DoHaltDatabase();
  gCanvas->SetContentNone();
  mDatabasePrefs.FromConfig();
  mDatabasePrefs.ToApp();
  Fill();
  RFDatabaseThread::DoStartDatabase();
}


/*****
Handle Factory Default button
*****/
void RFDatabasePanel::DoFactoryDefault()
{
  RFStatisticsThread::DoHaltStatistics();
  RFDatabaseThread::DoHaltDatabase();
  gCanvas->SetContentNone();
  RFDatabasePrefs factoryDefaults;
  factoryDefaults.ToConfig();
  mDatabasePrefs.FromConfig();
  mDatabasePrefs.ToApp();
  Fill();
  RFDatabaseThread::DoStartDatabase();
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
struct RFDisplayPrefs - settings that affect the displayed image
**********/

/*****
Constructor - default defines factory settings
*****/
RFDisplayPrefs::RFDisplayPrefs() :
  mClarifyVerbalAmbiguities(false),
  mAxiomsInVerbalDirections(false),
  mUnitPixels(80),
  mTextSize(10),
  mTextLeading(13),
  mDgmSpacing(10),
  mSearchNum(5),
  mNumBuckets(11),
  mBucketSize(0.001),
  mNumTrials(1000)
{
}


/*****
wxConfig keys
*****/
const wxString KEY_CLARIFY_VERBAL_AMBIGUITIES = wxT("ClarifyVerbalAmbiguities");
const wxString KEY_AXIOMS_IN_VERBAL_DIRECTIONS = wxT("AxiomsInVerbalDirections");
const wxString KEY_UNIT_PIXELS = wxT("UnitPixels");
const wxString KEY_TEXT_SIZE = wxT("TextSize");
const wxString KEY_TEXT_LEADING = wxT("TextLeading");
const wxString KEY_DGM_SPACING = wxT("DgmSpacing");
const wxString KEY_SEARCH_NUM = wxT("SearchNum");
const wxString KEY_NUM_BUCKETS = wxT("NumBuckets");
const wxString KEY_BUCKET_SIZE = wxT("BucketSize");
const wxString KEY_NUM_TRIALS = wxT("NumTrials");


/*****
Write current settings to Config file
*****/
void RFDisplayPrefs::ToConfig()
{
  wxConfig::Get()->Write(KEY_CLARIFY_VERBAL_AMBIGUITIES, mClarifyVerbalAmbiguities);
  wxConfig::Get()->Write(KEY_AXIOMS_IN_VERBAL_DIRECTIONS, mAxiomsInVerbalDirections);
  wxConfig::Get()->Write(KEY_UNIT_PIXELS, mUnitPixels);
  wxConfig::Get()->Write(KEY_TEXT_SIZE, mTextSize);
  wxConfig::Get()->Write(KEY_TEXT_LEADING, mTextLeading);
  wxConfig::Get()->Write(KEY_DGM_SPACING, mDgmSpacing);
  wxConfig::Get()->Write(KEY_SEARCH_NUM, mSearchNum);
  wxConfig::Get()->Write(KEY_NUM_BUCKETS, mNumBuckets);
  wxConfig::Get()->Write(KEY_BUCKET_SIZE, mBucketSize);
  wxConfig::Get()->Write(KEY_NUM_TRIALS, mNumTrials);
}


/*****
Read settings from Config file
*****/
void RFDisplayPrefs::FromConfig()
{
  RFDisplayPrefs defaults;
  wxConfig::Get()->Read(KEY_CLARIFY_VERBAL_AMBIGUITIES, &mClarifyVerbalAmbiguities, defaults.mClarifyVerbalAmbiguities);
  wxConfig::Get()->Read(KEY_AXIOMS_IN_VERBAL_DIRECTIONS, &mAxiomsInVerbalDirections, defaults.mAxiomsInVerbalDirections);
  wxConfig::Get()->Read(KEY_UNIT_PIXELS, &mUnitPixels, defaults.mUnitPixels);
  wxConfig::Get()->Read(KEY_TEXT_SIZE, &mTextSize, defaults.mTextSize);
  wxConfig::Get()->Read(KEY_TEXT_LEADING, &mTextLeading, defaults.mTextLeading);
  wxConfig::Get()->Read(KEY_DGM_SPACING, &mDgmSpacing, defaults.mDgmSpacing);
  wxConfig::Get()->Read(KEY_SEARCH_NUM, &mSearchNum, defaults.mSearchNum);
  wxConfig::Get()->Read(KEY_NUM_BUCKETS, &mNumBuckets, defaults.mNumBuckets);
  wxConfig::Get()->Read(KEY_BUCKET_SIZE, &mBucketSize, defaults.mBucketSize);
  wxConfig::Get()->Read(KEY_NUM_TRIALS, &mNumTrials, defaults.mNumTrials);
}


/*****
Write current values to ReferenceFinder static variables
*****/
void RFDisplayPrefs::ToApp()
{
  ReferenceFinder::sClarifyVerbalAmbiguities = mClarifyVerbalAmbiguities;
  ReferenceFinder::sAxiomsInVerbalDirections = mAxiomsInVerbalDirections;
  RFCanvas::sUnitPixels = mUnitPixels;
  RFCanvas::sTextSize = mTextSize;
  RFCanvas::sTextLeading = mTextLeading;
  RFCanvas::sDgmSpacing = mDgmSpacing;
  RFFrame::sSearchNum = mSearchNum;
  ReferenceFinder::sNumBuckets = mNumBuckets;
  ReferenceFinder::sBucketSize = mBucketSize;
  ReferenceFinder::sNumTrials = mNumTrials;
}


/*****
Read current values from ReferenceFinder and GUI static variables
*****/
void RFDisplayPrefs::FromApp()
{
  mClarifyVerbalAmbiguities = ReferenceFinder::sClarifyVerbalAmbiguities;
  mAxiomsInVerbalDirections = ReferenceFinder::sAxiomsInVerbalDirections;
  mUnitPixels = RFCanvas::sUnitPixels;
  mTextSize = RFCanvas::sTextSize;
  mTextLeading = RFCanvas::sTextLeading;
  mDgmSpacing = RFCanvas::sDgmSpacing;
  mSearchNum = RFFrame::sSearchNum;
  mNumBuckets = ReferenceFinder::sNumBuckets;
  mBucketSize = ReferenceFinder::sBucketSize;
  mNumTrials = ReferenceFinder::sNumTrials = mNumTrials;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFDisplayPanel - Preferences panel for display settings
**********/

/*****
Constructor
*****/
RFDisplayPanel::RFDisplayPanel(wxWindow* parent) :
  RFPrefsPanel(parent)
{
  RFStackSizer ss(new wxBoxSizer(wxVERTICAL), this);
  {
    RFStackSizer ss(new wxGridSizer(2, 2, 2), wxSizerFlags().Expand());
    {
      RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this,
          wxT("Layout")), wxSizerFlags().Expand());
        AddTextPair(wxT("Unit Pixels:"), mUnitPixels);
        AddTextPair(wxT("Text Size:"), mTextSize);
        AddTextPair(wxT("Text Spacing:"), mTextLeading);
        AddTextPair(wxT("Diagram Spacing:"), mDgmSpacing);
      }
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this,
          wxT("Verbal Directions")), wxSizerFlags().Expand());
        AddCheckBox(wxT("Clarify Verbal Ambiguities"), mClarifyVerbalAmbiguities);
        AddCheckBox(wxT("Note Axioms in Verbal Directions"), mAxiomsInVerbalDirections);
      }
    }
    {
      RFStackSizer ss(new wxBoxSizer(wxVERTICAL), wxSizerFlags().Expand());
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this,
          wxT("Search")), wxSizerFlags().Expand());
        AddTextPair(wxT("Number to Find:"), mSearchNum);
      }
      {
        RFStackSizer ss(new wxStaticBoxSizer(wxVERTICAL, this,
          wxT("Statistics")), wxSizerFlags().Expand());
        AddTextPair(wxT("Num Error Buckets:"), mNumBuckets);
        AddTextPair(wxT("Error Bucket Size:"), mBucketSize);
        AddTextPair(wxT("Number of Trials:"), mNumTrials);
      }
    }
  }
  mDisplayPrefs.FromApp();
  Fill();
}


/*****
Fill the display panel from static variables
*****/
void RFDisplayPanel::Fill()
{
  mSearchNum->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mSearchNum));
  mSearchNum->SetSelection(-1, -1);
  
  mClarifyVerbalAmbiguities->SetValue(mDisplayPrefs.mClarifyVerbalAmbiguities);
  mAxiomsInVerbalDirections->SetValue(mDisplayPrefs.mAxiomsInVerbalDirections);

  mUnitPixels->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mUnitPixels));
  mUnitPixels->SetSelection(-1, -1);
  mTextSize->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mTextSize));
  mTextSize->SetSelection(-1, -1);
  mTextLeading->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mTextLeading));
  mTextLeading->SetSelection(-1, -1);
  mDgmSpacing->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mDgmSpacing));
  mDgmSpacing->SetSelection(-1, -1);
  
  mNumBuckets->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mNumBuckets));
  mNumBuckets->SetSelection(-1, -1);
  mBucketSize->SetValue(wxString::Format(wxT("%.4f"), mDisplayPrefs.mBucketSize));
  mBucketSize->SetSelection(-1, -1);
  mNumTrials->SetValue(wxString::Format(wxT("%d"), mDisplayPrefs.mNumTrials));
  mNumTrials->SetSelection(-1, -1);
}


/*****
Read and validate the entries in the display panel, returning true if successful.
*****/
bool RFDisplayPanel::Read()
{
  if (!ReadInt(wxT("search number"), mSearchNum, mDisplayPrefs.mSearchNum, 0)) {
    mSearchNum->SetSelection(-1, -1);
    mSearchNum->SetFocus();
    return false;
  }
  mDisplayPrefs.mClarifyVerbalAmbiguities = mClarifyVerbalAmbiguities->GetValue();
  mDisplayPrefs.mAxiomsInVerbalDirections = mAxiomsInVerbalDirections->GetValue();
  if (!ReadInt(wxT("unit size"), mUnitPixels, mDisplayPrefs.mUnitPixels, 9)) {
    mUnitPixels->SetSelection(-1, -1);
    mUnitPixels->SetFocus();
    return false;
  }
  if (!ReadInt(wxT("text size"), mTextSize, mDisplayPrefs.mTextSize, 6)) {
    mTextSize->SetSelection(-1, -1);
    mTextSize->SetFocus();
    return false;
  }
  if (!ReadInt(wxT("text leading"), mTextLeading, mDisplayPrefs.mTextLeading, 6)) {
    mTextLeading->SetSelection(-1, -1);
    mTextLeading->SetFocus();
    return false;
  }
  if (!ReadInt(wxT("diagram spacing"), mDgmSpacing, mDisplayPrefs.mDgmSpacing, 0)) {
    mDgmSpacing->SetSelection(-1, -1);
    mDgmSpacing->SetFocus();
    return false;
  }
  if (!ReadInt(wxT("number of buckets"), mNumBuckets, mDisplayPrefs.mNumBuckets, 0)) {
    mNumBuckets->SetSelection(-1, -1);
    mNumBuckets->SetFocus();
    return false;
  }
  if (!ReadDouble(wxT("bucket size"), mBucketSize, mDisplayPrefs.mBucketSize, 0.0)) {
    mBucketSize->SetSelection(-1, -1);
    mBucketSize->SetFocus();
    return false;
  }
  if (!ReadInt(wxT("number of trials"), mNumTrials, mDisplayPrefs.mNumTrials, 0)) {
    mNumTrials->SetSelection(-1, -1);
    mNumTrials->SetFocus();
    return false;
  }

  return true;
}


/*****
Handle Apply button
*****/
void RFDisplayPanel::DoApply()
{
  if (Read()) {
    RFStatisticsThread::DoHaltStatistics();
    mDisplayPrefs.ToApp();
    gCanvas->SizeImageAndFrame();
  }
}


/*****
Handle Make Default button
*****/
void RFDisplayPanel::DoMakeDefault()
{
  if (Read()) {
    RFStatisticsThread::DoHaltStatistics();
    mDisplayPrefs.ToConfig();
    mDisplayPrefs.ToApp();
    gCanvas->SizeImageAndFrame();
  }
}


/*****
Handle Restore Default button
*****/
void RFDisplayPanel::DoRestoreDefault()
{
  RFStatisticsThread::DoHaltStatistics();
  mDisplayPrefs.FromConfig();
  mDisplayPrefs.ToApp();
  Fill();
  gCanvas->SizeImageAndFrame();
}


/*****
Handle Factory Default button
*****/
void RFDisplayPanel::DoFactoryDefault()
{
  RFStatisticsThread::DoHaltStatistics();
  RFDisplayPrefs factoryDefaults;
  factoryDefaults.ToConfig();
  mDisplayPrefs.FromConfig();
  mDisplayPrefs.ToApp();
  Fill();
  gCanvas->SizeImageAndFrame();
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFPrefsDialog - Preferences dialog
**********/

/*****
Constructor. Parent needs to be gFrame so that after the dialog is dismissed,
the frame gets a chance to update the menus.
*****/
RFPrefsDialog::RFPrefsDialog() : 
  wxDialog(gFrame, wxID_ANY, wxString(wxT("Preferences")))
{
  {
    RFStackSizer ss(new wxBoxSizer(wxVERTICAL), this);
    ss.Add(mNotebook = new wxNotebook(this, wxID_ANY),
      wxSizerFlags().Expand().Border(wxALL, 2));
    mNotebook->AddPage(mDatabasePanel = new RFDatabasePanel(mNotebook), wxT("Database"), true);
    mNotebook->AddPage(mDisplayPanel = new RFDisplayPanel(mNotebook), wxT("Display"));

    // Add row of buttons across the bottom
    RFStackSizer::AddSpacer(5);
    {
      RFStackSizer ss(new wxBoxSizer(wxHORIZONTAL), wxSizerFlags().Expand());
      wxButton *btnApply, *btnMakeDefault, *btnRestoreDefault, *btnFactoryDefault, *btnDone;
  
      ss.Add(btnFactoryDefault = new wxButton(this, RFID_FACTORY_DEFAULT, 
        wxT("Factory Default")),
        wxSizerFlags().Border(wxALL, 5));
      btnFactoryDefault->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
      ss.AddStretchSpacer();
  
      ss.Add(btnMakeDefault = new wxButton(this, RFID_MAKE_DEFAULT, 
        wxT("Make Default")),
        wxSizerFlags().Border(wxALL, 5));
      btnMakeDefault->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
      ss.AddStretchSpacer();
  
      ss.Add(btnRestoreDefault = new wxButton(this, RFID_RESTORE_DEFAULT, 
        wxT("Restore Default")),
        wxSizerFlags().Border(wxALL, 5));
      btnRestoreDefault->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
      ss.AddStretchSpacer();
  
      ss.Add(btnApply = new wxButton(this, RFID_APPLY, 
        wxT("Apply")),
        wxSizerFlags().Border(wxALL, 5));
      btnApply->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
      ss.AddStretchSpacer();
  
      ss.Add(btnDone = new wxButton(this, wxID_CANCEL, 
        wxT("Done")),
        wxSizerFlags().Border(wxALL, 5));
      btnDone->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
      btnDone->SetDefault();
    }
    RFStackSizer::AddSpacer(5);
  }
  Center(wxBOTH);
}


/*****
Handle Apply button
*****/
void RFPrefsDialog::OnApply(wxCommandEvent&)
{
  if (mNotebook->GetCurrentPage() == mDatabasePanel) 
    mDatabasePanel->DoApply();
  else if (mNotebook->GetCurrentPage() == mDisplayPanel)
    mDisplayPanel->DoApply();
  else RFFAIL("bad notebook page");
}


/*****
Handle Make Default button
*****/
void RFPrefsDialog::OnMakeDefault(wxCommandEvent&)
{
  if (mNotebook->GetCurrentPage() == mDatabasePanel) 
    mDatabasePanel->DoMakeDefault();
  else if (mNotebook->GetCurrentPage() == mDisplayPanel)
    mDisplayPanel->DoMakeDefault();
  else RFFAIL("bad notebook page");
}


/*****
Handle Restore Default button
*****/
void RFPrefsDialog::OnRestoreDefault(wxCommandEvent&)
{
  if (mNotebook->GetCurrentPage() == mDatabasePanel) 
    mDatabasePanel->DoRestoreDefault();
  else if (mNotebook->GetCurrentPage() == mDisplayPanel)
    mDisplayPanel->DoRestoreDefault();
  else RFFAIL("bad notebook page");
}


/*****
Handle Factory Default button
*****/
void RFPrefsDialog::OnFactoryDefault(wxCommandEvent&)
{
  if (mNotebook->GetCurrentPage() == mDatabasePanel) 
    mDatabasePanel->DoFactoryDefault();
  else if (mNotebook->GetCurrentPage() == mDisplayPanel)
    mDisplayPanel->DoFactoryDefault();
  else RFFAIL("bad notebook page");
}


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Event tables
*****/
BEGIN_EVENT_TABLE(RFPrefsDialog, wxDialog)
  EVT_BUTTON(RFID_APPLY, RFPrefsDialog::OnApply)
  EVT_BUTTON(RFID_MAKE_DEFAULT, RFPrefsDialog::OnMakeDefault)
  EVT_BUTTON(RFID_RESTORE_DEFAULT, RFPrefsDialog::OnRestoreDefault)
  EVT_BUTTON(RFID_FACTORY_DEFAULT, RFPrefsDialog::OnFactoryDefault)
END_EVENT_TABLE()
