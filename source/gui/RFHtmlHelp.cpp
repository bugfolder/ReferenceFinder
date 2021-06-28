/*******************************************************************************
File:         RFHtmlHelp.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation file for help classes
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-15
Copyright:    ©2006 Robert J. Lang. All Rights Reserved.
*******************************************************************************/

#include "RFHtmlHelp.h"
#include "RFApp.h"

/**********
class RFHtmlHelpController
Controls the display of help information and documentation
**********/

/*****
Constructor
*****/
RFHtmlHelpController::RFHtmlHelpController() : 
  wxHtmlHelpController(wxHF_TOOLBAR | wxHF_CONTENTS | wxHF_SEARCH | 
    wxHF_BOOKMARKS | wxHF_PRINT)
{
}


/*****
Return a help frame. We override this to return our own frame that allows
printing via menu command.
*****/
wxHtmlHelpFrame* RFHtmlHelpController::CreateHelpFrame(wxHtmlHelpData* data)
{
  // Duplicate inherited method, but use our own frame type, which can receive
  // print events via menu command and keyboard event.
  wxHtmlHelpFrame* helpFrame = new RFHtmlHelpFrame(data);
  helpFrame->SetController(this);
  helpFrame->Create(m_parentWindow, -1, wxEmptyString, m_FrameStyle, m_Config, m_ConfigRoot);
  helpFrame->SetTitleFormat(m_titleFormat);    
  m_helpFrame = helpFrame;

  // Add a menu bar that allows printing via menu item & key command
  helpFrame->SetMenuBar(gApp->MakeMenuBar());
    
  return helpFrame;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFHtmlHelpFrame
Displays help information and documentation and prints it
**********/

/*****
Constructor
*****/
RFHtmlHelpFrame::RFHtmlHelpFrame(wxHtmlHelpData* data) : 
  wxHtmlHelpFrame(data)
{
}


/*****
Handle an activate event
*****/
void RFHtmlHelpFrame::OnActivate(wxActivateEvent& event)
{
  wxHtmlHelpFrame::OnActivate(event);
  if (event.GetActive()) {
    gApp->SetTopWindow(this);
  }
}


/*****
Enable the File->Print... command
*****/
void RFHtmlHelpFrame::OnPrintUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(true);
}


/*****
Perform the File->Print... command
*****/
void RFHtmlHelpFrame::OnPrint(wxCommandEvent&)
{
  RFASSERT(!!GetHelpWindow()->GetHtmlWindow()->GetOpenedPage());
  gApp->GetHtmlEasyPrinting()->PrintFile(GetHelpWindow()->GetHtmlWindow()->GetOpenedPage());
}


/*****
Enable the File->Print Preview... command
*****/
void RFHtmlHelpFrame::OnPrintPreviewUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(true);
}


/*****
Perform the File->Print Preview... command
*****/
void RFHtmlHelpFrame::OnPrintPreview(wxCommandEvent& )
{
  RFASSERT(!!GetHelpWindow()->GetHtmlWindow()->GetOpenedPage());
  gApp->GetHtmlEasyPrinting()->PreviewFile(GetHelpWindow()->GetHtmlWindow()->GetOpenedPage());
}


/*****
Disable all other RF commands.
*****/
void RFHtmlHelpFrame::OnOtherUpdateUI(wxUpdateUIEvent& event)
{
  event.Enable(false);
}


/*****
Event table
*****/
BEGIN_EVENT_TABLE(RFHtmlHelpFrame, wxHtmlHelpFrame)
  EVT_ACTIVATE(RFHtmlHelpFrame::OnActivate)
  EVT_UPDATE_UI(wxID_PREVIEW, RFHtmlHelpFrame::OnPrintPreviewUpdateUI)
  EVT_MENU(wxID_PREVIEW, RFHtmlHelpFrame::OnPrintPreview)
  EVT_UPDATE_UI(wxID_PRINT, RFHtmlHelpFrame::OnPrintUpdateUI)
  EVT_MENU(wxID_PRINT, RFHtmlHelpFrame::OnPrint)
  EVT_UPDATE_UI_RANGE(RFID_EXPORT_PS, RFID_CALC_STATISTICS, RFHtmlHelpFrame::OnOtherUpdateUI)
END_EVENT_TABLE()
