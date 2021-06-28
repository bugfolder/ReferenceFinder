/******************************************************************************
File:         RFPrintout.h
Project:      ReferenceFinder 4.x
Purpose:      Header for printout
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-08
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/

#ifndef _RFPRINTOUT_H_
#define _RFPRINTOUT_H_

#include "RFPrefix.h"

#include "wx/print.h"

/**********
class RFPrintout
Implements printing of a crease pattern
**********/
class RFPrintout: public wxPrintout {
public:
  RFPrintout();
  virtual void OnPreparePrinting();
  virtual void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, 
    int *selPageTo);
  virtual bool HasPage(int pageNum);
  virtual bool OnPrintPage(int page);
};


#endif // _RFPRINTOUT_H_
