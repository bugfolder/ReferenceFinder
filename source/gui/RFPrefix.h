/******************************************************************************
ReferenceFinder - a program for finding compact folding sequences for locating 
approximate reference points on a unit square.

Copyright ©2003-2006 by Robert J. Lang. All rights reserved.

This program is built atop the wxWindows class library and its distribution
is subject to the wxWindows license; see www.wxwindows.org.
*******************************************************************************/
// Headers necessary for all ReferenceFinder/wxWindows classes
// This file is #included as a preprocessor prefix file

#ifndef _PREFIX_H_
#define _PREFIX_H_

/**********
wxWindows Headers
**********/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

/*
Definition of assertion macros for 4 levels of build: Debug, Development,
Profile, and Release. We use a single assertion macro: RFASSERT(cond).

Debug builds are built with wxWidgets debug libraries and use the wxASSERT() 
macro, which throws up a dialog and breaks into the debugger when an assertion
fails.
Debug builds should #define __WXDEBUG__ and should link to wxWidgets debug libs.

Development builds use the release version of wxWidgets but still include
assertion tests in ReferenceFinder code. Assertion failure puts up a dialog but
assumes we're not running in the debugger (i.e., a client is running the
program), so the program simply quits abnormally.
Development builds should #define RFDEBUG but not __WXDEBUG__
and should link to wxWidgets release libs.

Release builds compile out all assertion tests and don't #define either
RFDEBUG or __WXDEBUG__.
*/

/* RFDEBUG is defined for either Debug or Development builds */
#if defined(__WXDEBUG__) && !defined(RFDEBUG)
  #define RFDEBUG
#endif

/* RFASSERT macro is defined for both Debug or Development builds but behavior
  differs. */
#if defined(__WXDEBUG__)
  #define RFASSERT(cond) wxASSERT(cond)
  #define RFFAIL(msg) wxFAIL_MSG(msg)
#elif defined(RFDEBUG)
  void RFAssert(int cond, const wxChar* szFile, int nLine, const wxChar* szCond);
  #define RFASSERT(cond) RFAssert(!!(cond), __TFILE__, __LINE__, wxT(#cond))
  #define RFFAIL(msg) RFAssert(false, __TFILE__, __LINE__, wxT(msg))
#else
  #define RFASSERT(cond)
  #define RFFAIL(msg)
#endif


#endif // _PREFIX_H_
