/******************************************************************************
File:         RFApp.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for application class
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFApp.h"
#include "RFCanvas.h"
#include "RFFrame.h"
#include "RFHtmlHelp.h"
#include "RFPrefsDialog.h"
#include "RFStackSizer.h"
#include "RFThread.h"
#include "RFVersion.h"

#include "wx/statline.h"
#include "wx/config.h"
#include "wx/stdpaths.h"
#include "wx/splash.h"
#include "wx/image.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/wxhtml.h"
#include "wx/fs_zip.h"
#include "wx/cmdline.h"

// Need this for Macintosh Page Margins dialog
#ifdef __WXMAC__
#include "wx/mac/carbon/printdlg.h"
#endif // __WXMAC__

/* use GNOME's nice print dialog & engine if available (not necessary
   for compiling or running) */
#if wxUSE_LIBGNOMEPRINT
  #include "wx/html/forcelnk.h"
  FORCE_LINK(gnome_print)
#endif // wxUSE_LIBGNOMEPRINT

#include <sstream>

using namespace std;


/*****
global variable initialization
*****/
RFApp* gApp = 0;
RFFrame* gFrame = 0;
RFCanvas* gCanvas = 0;


/*****
Preference strings for wxConfig
*****/
const wxString SHOW_ABOUT_AT_STARTUP_KEY = wxT("ShowAboutAtStartup");


/*****
About dialog constructor
*****/
RFAboutDialog::RFAboutDialog() :
  wxDialog(NULL, wxID_ANY, wxT("About ReferenceFinder"), wxDefaultPosition, 
    wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
  {
    RFStackSizer ss(new wxBoxSizer(wxVERTICAL), this);
    wxStaticText* title = new wxStaticText(this, wxID_ANY, wxT(APP_V_M_B_NAME_STR));
    title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    ss.Add(title, wxSizerFlags().Center().Border(wxALL, 5));
    
    mAboutBox = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, 
      wxSize(600, 450));
    int fontSizes[7] = {9, 10, 11, 12, 14, 16, 18};
    mAboutBox->SetFonts(wxEmptyString, wxEmptyString, fontSizes);
    mAboutBox->SetBorders(20);
    wxFileName aboutFile(wxStandardPaths::Get().GetDataDir(), wxT("about.htm"));
    mAboutBox->LoadFile(aboutFile);
    ss.Add(mAboutBox);
    
    mShowAboutAtStartup = new wxCheckBox(this, wxID_ANY, 
      wxT("Show this window at startup"));
    int showAboutAtStartup;
    wxConfig::Get()->Read(SHOW_ABOUT_AT_STARTUP_KEY, &showAboutAtStartup, 1);
    mShowAboutAtStartup->SetValue(showAboutAtStartup);
    ss.Add(mShowAboutAtStartup, wxSizerFlags().Center().Border(wxTOP, 5));
    
    ss.Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags().Expand().Border(wxALL, 5));
    ss.GetSizer()->Add(CreateButtonSizer(wxOK), wxSizerFlags().Center().Border(wxALL, 5));
  
    wxStaticText* buildCode = new wxStaticText(this, wxID_ANY, wxT(BUILD_CODE_STR));
    buildCode->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, 
      wxFONTWEIGHT_NORMAL));
    ss.Add(buildCode, wxSizerFlags().Right());
  }
  Center(wxBOTH);
  return;
}


/*****
Note user choice with regard to showing the dialog at startup
*****/
bool RFAboutDialog::TransferDataFromWindow()
{
  int showAboutAtStartup = mShowAboutAtStartup->GetValue();
  wxConfig::Get()->Write(SHOW_ABOUT_AT_STARTUP_KEY, showAboutAtStartup);
  wxConfig::Get()->Flush();
  return true;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFApp - main application class
**********/

/*****
Constructor
*****/
RFApp::RFApp() :
  wxApp(),
  mPrintData(0),
  mPageSetupDialogData(0),
  mHtmlHelpController(0),
  mTimer(this)
{
  gApp = this;
}


/*****
Program initialization
*****/
bool RFApp::OnInit()
{
  if ( !wxApp::OnInit() ) return false;

  SetVendorName(wxT("langorigami.com"));
  SetAppName(wxT(APP_V_NAME_STR));

  CheckDirectoryPrefix();
  
  ::wxInitAllImageHandlers();
  wxFileSystem::AddHandler(new wxZipFSHandler());

#if defined(__WXMAC__) || defined(__LINUX__)
  // on Mac and Linux, we want the splash screen to appear first, but on MSW, a
  // problem arises if the About box is up when the splash screen appears, so
  // MSW has to wait until we've created the main frame, see below.
  if (!ShowSplashScreen()) return false;
#endif
    
#ifdef __LINUX__
  /* Load icon. Silently ignore failures (SetIcon will also silently fail) */
  mConfig.mAppIcon.LoadFile(wxStandardPaths::Get().GetDataDir() + 
    wxT("/Icon_app_48.png"));
#endif // __LINUX__

  if (!LoadHelp()) return false;

  mPrintData = new wxPrintData();
  mPageSetupDialogData = new wxPageSetupDialogData();
  (*mPageSetupDialogData) = (*GetPrintData());
  mPageSetupDialogData->SetMarginTopLeft(wxPoint(15, 15));
  mPageSetupDialogData->SetMarginBottomRight(wxPoint(15, 15));

  RFDatabasePrefs databasePrefs;
  databasePrefs.FromConfig();
  databasePrefs.ToApp();
  RFDisplayPrefs displayPrefs;
  displayPrefs.FromConfig();
  displayPrefs.ToApp();

  gFrame = new RFFrame(wxT("ReferenceFinder"));
  gCanvas->SetContentNone();
  gFrame->Show(true);
  
#ifdef __WXMSW__
  if (!ShowSplashScreen()) return false;
#endif

  RFDatabaseThread::DoStartDatabase();
  
  ShowOptionalAbout();
  
  mTimer.Start(333);  // check for status updates every 1/3 second

  return true;
}


/*****
Set directory prefix for auxiliary data files. Gives user a chance to run the
program even if installed differently than built under GNU/Linux.
*****/
void RFApp::CheckDirectoryPrefix()
{
#if defined(__LINUX__)
#ifndef INSTALL_PREFIX
#define INSTALL_PREFIX "/usr/local"
#endif
  wxString prefix;
  if (! mConfig.mInstallDir.IsEmpty ())
    prefix = mConfig.mInstallDir;
  else {
    char *p = getenv ("REFERENCEFINDER_PREFIX");
    if (p)
      prefix = wxT (p);
    else
      prefix = wxT (INSTALL_PREFIX);
  }
  dynamic_cast<wxStandardPaths &> (wxStandardPaths::Get()).
    SetInstallPrefix(prefix);
#endif // defined(__LINUX__)
}


/*****
Show the splash screen (but not in debug builds because we quickly tire of it
while debugging).
*****/
bool RFApp::ShowSplashScreen()
{
  wxFileName splashFile(wxStandardPaths::Get().GetDataDir(), 
    wxT("SplashScreen.png"));
  if (splashFile.IsOk() && splashFile.FileExists()) {
#ifndef RFDEBUG
    const int SPLASH_DURATION = 3000;
    wxBitmap* splashScreenBitmap = 
      new wxBitmap(splashFile.GetFullPath(), wxBITMAP_TYPE_PNG);
    new wxSplashScreen(*splashScreenBitmap,
      wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
      SPLASH_DURATION, NULL, wxID_ANY, wxDefaultPosition, wxDefaultSize,
      wxSIMPLE_BORDER | wxSTAY_ON_TOP);
#endif // RFDEBUG
  }
  else {
    wxString msg = wxString::Format(wxT(\
      "Splash screen file \"%s\" cannot be found. Please reinstall "\
      "ReferenceFinder 4."), splashFile.GetFullPath().c_str());
    wxMessageBox(msg, wxT("Startup Error"), wxOK | wxICON_ERROR);
    return false;
  }
  // Note: this wxYield is needed on some platforms to process the splash
  // screen and on Mac to process AppleEvents generated at startup.
  wxYield();
  return true;
}


/*****
Setup the HTML Help controller, load the help archive, and setup location for
help cache.
*****/
bool RFApp::LoadHelp()
{
  mHtmlEasyPrinting = new wxHtmlEasyPrinting;
  mHtmlEasyPrinting->SetFooter(
    wxT("<hr><p align=\"right\">page @PAGENUM@ of @PAGESCNT@</p>"), 
    wxPAGE_ALL);
  int fontsizes[] = { 6, 8, 10, 12, 14, 16, 18 }; 
  mHtmlEasyPrinting->SetFonts(wxEmptyString, wxEmptyString, fontsizes);
  mHtmlHelpController = new RFHtmlHelpController();
  mHtmlHelpController->UseConfig(wxConfig::Get());
  wxFileName helpFileName(wxStandardPaths::Get().GetDataDir(), wxT("help.zip"));
  if (!helpFileName.IsOk() || !helpFileName.FileExists()) {
    wxMessageBox(wxT("Couldn't load help file"), wxT("Initialization Error"));
    return false;
  }
  // application might be read-only so we'll put cached files in the user data
  // dir, which we might have to create if it doesn't yet exist. And if we're
  // unable to create it, then we just won't use a cache for the help data.
  wxString userDataDir = wxStandardPaths::Get().GetUserLocalDataDir();

#if __LINUX__
  /* In Linux/Unix, by default wxConfig instantiates the default
     wxFileConfig set to save the user's local configuration in a file
     in his/her home directory, named "." + GetAppName () - which
     happens to be the same path as returned by GetUserLocalDataDir
     above. To avoid conflicts, add a suffix to the help cache
     directory's name. */
  userDataDir.append (".cache");
#endif

  if (!wxFileName::DirExists(userDataDir)) {
    wxFileName::Mkdir(userDataDir); 
  }
  if (wxFileName::DirExists(userDataDir)) {
    mHtmlHelpController->SetTempDir(userDataDir);
  }
  else
    mHtmlHelpController->SetTempDir(wxEmptyString);
  bool err = mHtmlHelpController->AddBook(helpFileName);
  if (!err) {
    wxLogError(wxT("Initialization error: couldn't add help book."));
    return false;
  }
  return true;
}


/*****
Get our first-run setting and if appropriate, show the about dialog. Default
value is true (if there's no wxConfig, this really is the first run).
*****/
void RFApp::ShowOptionalAbout()
{
  int showAboutAtStartup;
  wxConfig::Get()->Read(SHOW_ABOUT_AT_STARTUP_KEY, &showAboutAtStartup, 1);
  if (showAboutAtStartup) {
    RFAboutDialog dialog;
    dialog.ShowModal();
  }
}


/*****
Initialize command-line switches that we accept
*****/
void RFApp::OnInitCmdLine(wxCmdLineParser &argParser) {
  argParser.AddSwitch (wxT("v"), wxT("version"), 
           wxT("show program version"));
  argParser.AddSwitch (wxT("h"), wxT("help"), 
           wxT("show option list"), wxCMD_LINE_OPTION_HELP);
  argParser.AddOption(wxT("d"), wxT("datadir"),
          wxT("set data directory path prefix"), 
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}


/*****
Respond to command-line arguments after the command line has been parsed.
*****/
bool RFApp::OnCmdLineParsed(wxCmdLineParser& parser) {
  wxString par;
  if (parser.Found (wxT("v"))) {
    std::cout << APP_V_M_B_NAME_STR << "\n";
    exit (0);
  }
  if (parser.Found (wxT("d"), &par))
    mConfig.mInstallDir = par;
  // parser is local to base method, so must copy non-option parameters here
  size_t nFiles = parser.GetParamCount();
  for (size_t i = 0; i < nFiles; i++)
    mConfig.mArgs.Add (parser.GetParam(i));

  return true;
}


/*****
Append an item to a menu with no menu icon
*****/
void RFApp::AppendPlainItem(wxMenu* menu, int id, const wxString& label, 
  const wxString& help)
{
  menu->Append(new wxMenuItem(menu, id, label, help));
}


/*****
Append an item to a menu, supplying the wxArtProvider ID for a standard icon.
*****/
void RFApp::AppendDecoratedItem(wxMenu* menu, int id, const wxString& label, 
  const wxString& help, const wxArtID& bmpid)
{
  wxMenuItem *item = new wxMenuItem(menu, id, label, help);
#ifndef __WXMAC__
  item->SetBitmap(wxArtProvider::GetBitmap(bmpid, wxART_MENU));
#endif // __WXMAC__
  menu->Append(item);
}


/*****
Create a menu bar for any frame (used for main frame and Help frame)
*****/
wxMenuBar* RFApp::MakeMenuBar()
{
  // File menu
  wxMenu *fileMenu = new wxMenu;
  AppendPlainItem(fileMenu, RFID_EXPORT_PS, 
    wxT("Export PostScript...\tCtrl+E"),
    wxT("Export currently-displayed diagrams as a PostScript file"));
  fileMenu->AppendSeparator();
  AppendPlainItem(fileMenu, wxID_PREFERENCES, // Mac will move this item
    wxT("Preferences..."),
    wxT("Set the preferences for display and for the reference database"));
#ifndef __WXMAC__
  fileMenu->AppendSeparator();
#endif // __WXMAC__
  AppendDecoratedItem(fileMenu, wxID_PRINT, 
    wxT("&Print\tCtrl+P"),
    wxT("Print the currently-displayed diagrams"),
    wxART_PRINT);
#ifdef __WXMAC__
  AppendPlainItem(fileMenu, RFID_PAGE_MARGINS, 
    wxT("Page Margins..."),
    wxT("Set the page margins"));
#endif // __WXMAC__
  AppendPlainItem(fileMenu, wxID_PRINT_SETUP, 
    wxT("Page &Setup..."),
    wxT("Set up the page for printing"));
  AppendPlainItem(fileMenu, wxID_PREVIEW, 
    wxT("Print Pre&view...\tCtrl+Alt+P"),
    wxT("Preview the print image before printing"));
#ifndef __WXMAC__
  fileMenu->AppendSeparator();
#endif // __WXMAC__
  fileMenu->Append(wxID_EXIT, wxT("E&xit\tAlt+X"), // Mac will move this item
    wxT("Quit this program"));
  
  // Edit menu
  wxMenu* editMenu = new wxMenu;
  AppendDecoratedItem(editMenu, wxID_UNDO, 
    wxT("Undo\tCtrl+Z"),
    wxT("Undo the previous action"),
    wxART_UNDO);
  editMenu->AppendSeparator();
  AppendDecoratedItem(editMenu, wxID_CUT, 
    wxT("Cut\tCtrl+X"),
    wxT("Cut the selection to the Clipboard"),
    wxART_CUT);
  AppendDecoratedItem(editMenu, wxID_COPY, 
    wxT("Copy\tCtrl+C"),
    wxT("Copy the selection to the Clipboard"),
    wxART_COPY);
  AppendDecoratedItem(editMenu, wxID_PASTE,
    wxT("Paste\tCtrl+V"),
    wxT("Paste the current Clipboard selection"),
    wxART_PASTE);
  AppendPlainItem(editMenu, wxID_CLEAR,
    wxT("Clear"),
    wxT("Clear the selection"));
    
  // Action menu
  wxMenu* actionMenu = new wxMenu;
  AppendPlainItem(actionMenu, RFID_TOGGLE_MARKS_LINES, 
    wxT("Seek Marks/Lines\tCtrl+T"),
    wxT("Toggle the main window between looking for marks versus lines"));
  actionMenu->AppendSeparator();
  AppendPlainItem(actionMenu, RFID_GET_REFERENCES, 
    wxT("Get References\tCtrl+G"),
    wxT("Search for folding sequences for the specified reference"));
  AppendPlainItem(actionMenu, RFID_CLEAR_REFERENCES, 
    wxT("Clear References\tCtrl+Alt+G"),
    wxT("Clear the currently-displayed references"));
  AppendPlainItem(actionMenu, RFID_REBUILD, 
    wxT("Rebuild Database\tCtrl+R"),
    wxT("Rebuild the database of marks and lines"));
   AppendPlainItem(actionMenu, RFID_CALC_STATISTICS, 
    wxT("Calculate Statistics\tCtrl+I"),
    wxT("Calculate statistics on the current database."));
  actionMenu->AppendSeparator();
  AppendPlainItem(actionMenu, RFID_HALT_REBUILD, 
#ifdef __WXMAC__
    // On Mac, cmd-period is the universal halt key combination, and cmd-H is
    // predefined at the system level to hide the application.
    wxT("Halt Calculation\tCtrl+."),
#else
    wxT("Halt Calculation\tCtrl+H"),
#endif
    wxT("Halt rebuilding of the database and/or statistical analysis."));
 
  
  // Help menu
  wxMenu *helpMenu = new wxMenu;
  AppendPlainItem(helpMenu, wxID_ABOUT, // Mac will move this item
    wxT("&About...")
#ifndef __WXMAC__
      wxT("\tF1")
#endif
      , 
    wxT("Show about dialog"));
  AppendPlainItem(helpMenu, wxID_HELP, 
    wxT("&Help...\tCtrl+?"), 
    wxT("Show the application documentation"));

  // Menu bar
  wxMenuBar *menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, wxT("&File"));
  menuBar->Append(editMenu, wxT("&Edit"));
  menuBar->Append(actionMenu, wxT("&Action"));
  menuBar->Append(helpMenu, wxT("&Help"));
  return menuBar;
}


/*****
Cleanup on exit
*****/
int RFApp::OnExit()
{
  mTimer.Stop();
  RFDatabaseThread::DoHaltDatabase();
  return wxApp::OnExit();
}


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Handle the About menu item
*****/
void RFApp::OnAbout(wxCommandEvent&)
{
  RFAboutDialog dialog;
  dialog.ShowModal();
}


/*****
Handle the Help menu item
*****/
void RFApp::OnHelp(wxCommandEvent&)
{
  mHtmlHelpController->DisplayContents();
}


/*****
Handle Preferences menu item
*****/
void RFApp::OnPrefs(wxCommandEvent&)
{
  RFPrefsDialog dialog;
  dialog.ShowModal();
}


/*****
Put up a dialog for setting the page margins
*****/
#ifdef __WXMAC__
void RFApp::OnPageMargins(wxCommandEvent& WXUNUSED(event))
{
  wxMacPageMarginsDialog dialog(gFrame, GetPageSetupDialogData());
  dialog.ShowModal();
  (*mPrintData) = dialog.GetPageSetupDialogData().GetPrintData();
  (*mPageSetupDialogData) = dialog.GetPageSetupDialogData();
}
#endif // __WXMAC__


/*****
Put up a dialog for setting printer parameters.
*****/
void RFApp::OnPrintSetup(wxCommandEvent& WXUNUSED(event))
{
  wxPageSetupDialog dialog(gFrame, GetPageSetupDialogData());
  dialog.ShowModal();
  (*mPrintData) = dialog.GetPageSetupDialogData().GetPrintData();
  (*mPageSetupDialogData) = dialog.GetPageSetupDialogData();
}


/*****
Respond to the Quit command
*****/
void RFApp::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    mTimer.Stop();
    mHtmlHelpController->Quit();
    gFrame->Close(true);
}


/*****
Process a timer event. If the thread status info has changed, we force a 
refresh of the display window.
*****/
void RFApp::OnTimer(wxTimerEvent&)
{
  static RFThread::DatabaseInfo lastDatabaseInfo;
  static RFThread::StatisticsInfo lastStatisticsInfo;
  RFThread::DatabaseInfo databaseInfo = RFThread::GetDatabaseInfo();
  RFThread::StatisticsInfo statisticsInfo = RFThread::GetStatisticsInfo();
  if (lastDatabaseInfo != databaseInfo || lastStatisticsInfo != statisticsInfo) {
    // Special case: when statistics is done calculating, we need to resize the
    // frame to hold the results.
    if (lastStatisticsInfo.mStatus != ReferenceFinder::STATISTICS_DONE &&
      statisticsInfo.mStatus == ReferenceFinder::STATISTICS_DONE) {
      if (gCanvas) gCanvas->SetContentStatistics();
    }
    if (gFrame) gFrame->Refresh();
  }
  lastDatabaseInfo = databaseInfo;
  lastStatisticsInfo = statisticsInfo;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Event tables
*****/
BEGIN_EVENT_TABLE(RFApp, wxApp)
  EVT_MENU(wxID_ABOUT, RFApp::OnAbout)
  EVT_MENU(wxID_HELP, RFApp::OnHelp)
  EVT_MENU(wxID_PREFERENCES, RFApp::OnPrefs)
#ifdef __WXMAC__
  EVT_MENU(RFID_PAGE_MARGINS, RFApp::OnPageMargins)
#endif // __WXMAC__
  EVT_MENU(wxID_PRINT_SETUP, RFApp::OnPrintSetup)
  EVT_MENU(wxID_EXIT, RFApp::OnQuit)
  EVT_TIMER(wxID_ANY, RFApp::OnTimer)
END_EVENT_TABLE()


/*****
Application implementation
*****/
IMPLEMENT_APP(RFApp)
