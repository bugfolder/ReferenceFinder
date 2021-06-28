/*******************************************************************************
File:         RFHtmlHelp.h
Project:      ReferenceFinder 4.x
Purpose:      Header file for help classes
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-15
Copyright:    ©2006 Robert J. Lang. All Rights Reserved.
*******************************************************************************/

#ifndef _RFHTMLHELP_H_
#define _RFHTMLHELP_H_

#include "RFPrefix.h"

#include "wx/html/helpctrl.h"
#include "wx/html/helpfrm.h"

class RFHtmlHelpFrame;

/**********
class RFHtmlHelpController
Controls the display of help information and documentation
**********/

class RFHtmlHelpController: public wxHtmlHelpController
{
public:
  RFHtmlHelpController();
  wxHtmlHelpFrame* CreateHelpFrame(wxHtmlHelpData* data);
};


/**********
class RFHtmlHelpFrame
Displays help information and documentation and prints it
**********/

class RFHtmlHelpFrame: public wxHtmlHelpFrame
{
public:
  RFHtmlHelpFrame(wxHtmlHelpData* data);
  
  // Event handling
  void OnActivate(wxActivateEvent& event);
  void OnPrintUpdateUI(wxUpdateUIEvent& event);
  void OnPrint(wxCommandEvent& event);
  void OnPrintPreviewUpdateUI(wxUpdateUIEvent& event);
  void OnPrintPreview(wxCommandEvent& event);
  void OnOtherUpdateUI(wxUpdateUIEvent& event);
  
private:
  DECLARE_EVENT_TABLE()
};


#endif // _RFHTMLHELP_H_
