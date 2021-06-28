/******************************************************************************
File:         RFStackSizer.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for stack-based wxSizer class
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-04
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFStackSizer.h"

/* Notes on class RFStackSizer.
This class is a "helper" class for working with sizers that lets you use the 
C++ scoping mechanism to cut down the amount of typing and creation of temporary
variables needed when creating deeply nested sets of sizers. 

The basic idea is that there is always a "current sizer", so that when you call
RFStackSizer::Add(..), the thing being added (a window or extra space) is added
to the current sizer. When you want to add a sizer to the current sizer, create
a new scope block and create a new RFStackSizer inside the block.

MORE TO COME
*/


/**********
class RFStackSizer -- a stack-based helper class for building windows with
nested sizers.
**********/

/*****
Static members
*****/
wxSizer* RFStackSizer::sCurSizer = 0;


/*****
Constructor for sizer that is assigned to a window.
*****/
RFStackSizer::RFStackSizer(wxSizer* sizer, wxWindow* window)
{
  mPrevSizer = sCurSizer;
  sCurSizer = sizer;
  mWindow = window;
}


/*****
Constructor for sizer that gets added to the currently-active sizer
*****/
RFStackSizer::RFStackSizer(wxSizer* sizer, wxSizerFlags flags)
{
  if (sCurSizer) sCurSizer->Add(sizer, flags);
  mPrevSizer = sCurSizer; 
  sCurSizer = sizer;
  mWindow = 0;
}


/*****
Destructor: restore the currently-active sizer to what it was when we came
in scope. If this was constructed with a wxWindow, set it to be the window's sizer.
*****/
RFStackSizer::~RFStackSizer()
{
  if (mWindow) mWindow->SetSizerAndFit(sCurSizer);
  sCurSizer = mPrevSizer;
}


/*****
Return the currently-active sizer
*****/
wxSizer* RFStackSizer::GetSizer()
{
  return sCurSizer;
}


/*****
Add a window to the currently-active sizer
*****/
void RFStackSizer::Add(wxWindow* window, wxSizerFlags flags) 
{
  sCurSizer->Add(window, flags);
}


/*****
Add some fixed space to the currently-active sizer
*****/
void RFStackSizer::AddSpacer(int s)
{
  sCurSizer->AddSpacer(s);
}


/*****
Add stretchable space to the currently active sizer
*****/
void RFStackSizer::AddStretchSpacer()
{
  sCurSizer->AddStretchSpacer();
}
