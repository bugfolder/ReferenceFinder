/******************************************************************************
File:         RFPrintout.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for printout
Author:       Robert J. Lang
Modified by:  
Created:      2006-05-08
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/

#include "RFPrintout.h"
#include "RFApp.h"
#include "RFCanvas.h"

/* Notes on printout.
The only thing that gets printed are the diagrams that are shown in the main
display window. Each folding sequence appears as a single row of diagrams with
text underneath; this is a "block." In the printout, we add user-settable page
margins all the way around (given in printer points); then the diagrams are
scaled so that the widest block of diagrams fills up the full width of the page
within the margins. The diagrams are printed with multiple blocks per page, page
breaks only between blocks.

To make this work, there is a little handshaking between RFPrintout and
RFCanvas. Namely, RFCanvas::mImageBlocks contains a list of the heights (in
pixels) of each block of diagrams. This array is set during a calibration call
to RFCanvas::DoDrawRefs(). The other member of RFCanvas relevant to pagination
is RFCanvas::mBlockPages, which contains the page on which each block of
diagrams appears. This array, although owned by RFCanvas, is set by RFPrintout
at the beginning of printing; essentially, this is where pagination happens.
*/

/**********
class RFPrintout
**********/

/*****
Constructor
*****/
RFPrintout::RFPrintout()
  : wxPrintout(gApp->GetAppName())
{
}


/*****
OVERRIDE
Called by the framework to configure the printout. Paginates the set of diagrams
by setting (in RFCanvas::mBlockPages) the number indicating the page that each
block of diagrams gets printed on. Since pagination has to work for both preview
and printed image, compute pagination normalized to the page width -- which
means that we're using the vertical aspect ratio of the page margins rect and of
the blocks of diagram. We only break pages between complete blocks of diagrams.
*****/
void RFPrintout::OnPreparePrinting()
{
  // We do all our drawing in device coordinates so that drawing is done at
  // maximum device resolution on the printed page.
  MapScreenSizeToDevice();
  wxRect pmRect = GetLogicalPageMarginsRect(*gApp->GetPageSetupDialogData());
  double pageAR = double(pmRect.height) / pmRect.width;
  
  // Paginate the blocks of diagrams
  gCanvas->mBlockPages.clear();
  double curAR = 0.0;
  int curPage = 1;
  for (size_t i = 0; i < gCanvas->mImageBlocks.size(); ++i) {
    double blockAR = double(gCanvas->mImageBlocks[i]) / gCanvas->mImageWidth;
    if (curAR + blockAR <= pageAR) {
      curAR += blockAR;
    }
    else {
      curPage++;
      curAR = blockAR;
    }
    gCanvas->mBlockPages.push_back(curPage);
  }
  
}


/*****
Called by the framework to obtain information about page range.
*****/
void RFPrintout::GetPageInfo(int* minPage, int* maxPage, int* selPageFrom, 
  int* selPageTo)
{
  *minPage = 1;
  *maxPage = gCanvas->mBlockPages.back();
  *selPageFrom = 1;
  *selPageTo = *maxPage;
}


/*****
Return true if the printout can provide the given page.
*****/
bool RFPrintout::HasPage(int pageNum)
{
  if (pageNum <= gCanvas->mBlockPages.back()) return true;
  return false;
}


/*****
Print a single page of the document.
*****/
bool RFPrintout::OnPrintPage(int page)
{
  // Set the DC scale to map the total image width to the page margins rect
  // width and put the origin at the top left of the page margins rect.
  wxRect pmRect = GetLogicalPageMarginsRect(*gApp->GetPageSetupDialogData());
#ifdef __WXGTK__
  /* With wxGTK 2.8.0rc1 (print code @ CVS @ 061118), each page's
     image appears more and more offset towards the top. It's probably
     a wx issue, so this is a temporary fix - CAF
   */
  OffsetLogicalOrigin(pmRect.x, -page * pmRect.y);
#else
  OffsetLogicalOrigin(pmRect.x, pmRect.y);
#endif
  
  gCanvas->mDCScale = double(pmRect.width) / gCanvas->mImageWidth;
  gCanvas->mIsPrinting = true;
  gCanvas->mPrintPage = page;
  gCanvas->DoDraw(*GetDC());
  gCanvas->mIsPrinting = false;
  return true;
}
