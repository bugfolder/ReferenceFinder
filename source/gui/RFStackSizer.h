/******************************************************************************
File:         RFStackSizer.h
Project:      ReferenceFinder 4.x
Purpose:      Header for stack-based wxSizer class
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-04
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFSTACKSIZER_H_
#define _RFSTACKSIZER_H_

#include "RFPrefix.h"

#include "wx/sizer.h"

/**********
class RFStackSizer -- a stack-based helper class for building windows with
nested sizers.
**********/

class RFStackSizer {
public:
  RFStackSizer(wxSizer* sizer, wxWindow* window);
  RFStackSizer(wxSizer* sizer, wxSizerFlags flags = wxSizerFlags());
  ~RFStackSizer();
  
  static wxSizer* GetSizer();
  static void Add(wxWindow* window, wxSizerFlags flags = wxSizerFlags());
  static void AddSpacer(int s);
  static void AddStretchSpacer();

private:
  wxSizer* mPrevSizer;
  wxWindow* mWindow;
  static wxSizer* sCurSizer;
};

#endif // _RFSTACKSIZER_H_
