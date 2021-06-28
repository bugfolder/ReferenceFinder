/******************************************************************************
File:         RFApp.h
Project:      ReferenceFinder 4.x
Purpose:      Header for application class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFAPP_H_
#define _RFAPP_H_

#include "RFPrefix.h"

#include "ReferenceFinder.h"

#include "wx/app.h"
#include "wx/artprov.h"

class RFApp;
class RFFrame;
class RFCanvas;
class RFPrefsDialog;

class wxHtmlWindow;
class wxHtmlHelpController;
class wxHtmlEasyPrinting;

/**********
Menu, command, and window IDs
**********/
enum {
  // Commands
  RFID_EXPORT_PS = 2000,
  RFID_TOGGLE_MARKS_LINES,
  RFID_GET_REFERENCES,
  RFID_CLEAR_REFERENCES,
  RFID_REBUILD,
  RFID_HALT_REBUILD,
  RFID_CALC_STATISTICS,
  RFID_PAGE_MARGINS,
  // Radio buttons in main window
  RFID_POINT,
  RFID_LINE,
  // Preferences buttons
  RFID_APPLY,
  RFID_MAKE_DEFAULT,
  RFID_RESTORE_DEFAULT,
  RFID_FACTORY_DEFAULT,
  RFID_LAST
};


/*****
global variables
*****/
extern RFApp* gApp;
extern RFFrame* gFrame;
extern RFCanvas* gCanvas;


/**********
class RFAboutDialog
Dialog displaying information about the program
**********/
class RFAboutDialog : public wxDialog {
public:
  RFAboutDialog();
  bool TransferDataFromWindow();
private:
  wxHtmlWindow* mAboutBox;
  wxCheckBox* mShowAboutAtStartup;
};


/**********
class RFApp - main application class
**********/
class RFApp : public wxApp {
public:
  RFApp();
  virtual bool OnInit();
  void CheckDirectoryPrefix();
  bool ShowSplashScreen();
  bool LoadHelp();
  void ShowOptionalAbout();
  void OnInitCmdLine(wxCmdLineParser& parser);
  bool OnCmdLineParsed(wxCmdLineParser& parser);
  void AppendPlainItem(wxMenu* menu, int id, const wxString& label, 
    const wxString& help);
  void AppendDecoratedItem(wxMenu* menu, int id, const wxString& label, 
    const wxString& help, const wxArtID& bmpid);
  wxMenuBar* MakeMenuBar();
  virtual int OnExit();
  
  // Misc getters
  wxPrintData* GetPrintData() { return mPrintData; }
  wxPageSetupDialogData* GetPageSetupDialogData() { return mPageSetupDialogData; }
  wxHtmlEasyPrinting* GetHtmlEasyPrinting() { return mHtmlEasyPrinting; }

  // event handling
  void OnAbout(wxCommandEvent& event);
  void OnHelp(wxCommandEvent& event);
  void OnPrefs(wxCommandEvent& event);
  void OnPageMargins(wxCommandEvent& event);
  void OnPrintSetup(wxCommandEvent& event);
  void OnQuit(wxCommandEvent& event);
  void OnTimer(wxTimerEvent& event);

private:
  wxPrintData* mPrintData;
  wxPageSetupDialogData* mPageSetupDialogData;
  wxHtmlHelpController* mHtmlHelpController;
  wxHtmlEasyPrinting* mHtmlEasyPrinting;
  wxTimer mTimer;
  struct { // runtime configuration/parameters
    wxString mInstallDir; // if ! empty, installation directory
    wxArrayString mArgs; // copy of non-option cmdline arguments
#if defined(__LINUX__) || defined(__WXMSW__)
    wxIcon mAppIcon; // icon for application top-level frame (may be ! Ok())
#endif // defined(__LINUX__) || defined(__WXMSW__)
  } mConfig;

  DECLARE_EVENT_TABLE()
};

DECLARE_APP(RFApp)


#endif // _RFAPP_H_
