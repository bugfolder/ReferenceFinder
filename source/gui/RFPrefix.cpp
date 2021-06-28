/******************************************************************************
ReferenceFinder - a program for finding compact folding sequences for locating 
approximate reference points on a unit square.

Copyright ©2003-2006 by Robert J. Lang. All rights reserved.

This program is built atop the wxWindows class library and its distribution
is subject to the wxWindows license; see www.wxwindows.org.
*******************************************************************************/
#include "RFPrefix.h"

#if !defined(__WXDEBUG__) && defined(RFDEBUG)

#include <cstdlib>

/*****
Put up a dialog that displays the failed assertion, then exit the program.
*****/
void RFAssert(int cond, const wxChar* szFile, int nLine, const wxChar* szCond)
{
  if (cond) return;
  wxString msg = wxT("Failed Assertion in file;");
  msg += szFile;
  msg += wxString::Format(wxT(" at line %d. Failed condition was:\n"), nLine);
  msg += szCond;
  ::wxMessageBox(msg, wxT("Failed Assertion"));
  abort();
}
#endif // !defined(__WXDEBUG__) && defined(RFDEBUG)
