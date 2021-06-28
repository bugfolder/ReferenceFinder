/******************************************************************************
File:         ReferenceFinder.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for ReferenceFinder generic model
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-22
Copyright:    ©1999-2007 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "ReferenceFinder.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace std;


/******************************************************************************
Section 1: Class ReferenceFinder and client interface
******************************************************************************/

/**********
class ReferenceFinder - object that builds and maintains collections of marks
and lines and can search throught the collection for marks and lines close to a
target mark or line. This class is the primary interface to the ReferenceFinder
database.
**********/

/*****
Accessible static member initialization -- variables that you might want to
change (because they alter the behavior of the program) are given default
values here. Less-accessible static member variables are defined lower down.
*****/

// Use unit square paper
Paper ReferenceFinder::sPaper(1.0, 1.0);

// These are switches by which we can turn on and off the use of different
// types of alignments. Default is to use all possible.
bool ReferenceFinder::sUseRefLine_C2P_C2P = true;
bool ReferenceFinder::sUseRefLine_P2P = true;
bool ReferenceFinder::sUseRefLine_L2L = true;
bool ReferenceFinder::sUseRefLine_L2L_C2P = true;
bool ReferenceFinder::sUseRefLine_P2L_C2P = true;
bool ReferenceFinder::sUseRefLine_P2L_P2L = true;
bool ReferenceFinder::sUseRefLine_L2L_P2L = true;

// Maximum rank and number of marks and lines to collect. These can be tweaked
// up or down to trade off accuracy versus memory and initialization time.
ReferenceFinder::rank_t ReferenceFinder::sMaxRank = 6;
size_t ReferenceFinder::sMaxLines = 500000;
size_t ReferenceFinder::sMaxMarks = 500000;

// constants that quantify the discretization of marks and lines in forming
// keys. The maximum key has the value (sNumX * sNumY) for marks, (sNumA * sNumD)
// for lines. These numbers set a limit on the accuracy, since we won't create
// more than one object for a given key.
int ReferenceFinder::sNumX = 5000;
int ReferenceFinder::sNumY = 5000;
int ReferenceFinder::sNumA = 5000;
int ReferenceFinder::sNumD = 5000;
  
// Defines "good enough" for a mark. For marks with errors better than this, we
// give priority to lower-rank marks.
double ReferenceFinder::sGoodEnoughError = .005;

// Minimum allowable aspect ratio for a flap. Too skinny of a flap can't be
// folded accurately.
double ReferenceFinder::sMinAspectRatio = 0.100;

// Sine of minimum intersection angle between creases that define a new mark.
// If the lines are close to parallel, the mark is imprecise.
double ReferenceFinder::sMinAngleSine = 0.342; // = sin(20 degrees)

// If sVisibilityMatters == true, we don't construct alignments that can't be
// made with opaque paper. Otherwise, we allow alignments that can be done with
// translucent paper.
bool ReferenceFinder::sVisibilityMatters = true;

// If sLineWorstCaseError == true, we use worst-case error to sort lines rather
// than Pythagorean error. The former is more accurate, but slows searches.
bool ReferenceFinder::sLineWorstCaseError = true;

// We make a call to our show progress callback routine every sDatabaseStatusSkip
// attempts.
int ReferenceFinder::sDatabaseStatusSkip = 200000;

// If sClarifyVerbalAmbiguities == true, then verbal instructions that could be
// ambigious because there are multiples solutions are clarified with
// additional information.
bool ReferenceFinder::sClarifyVerbalAmbiguities = true;

// If sAxiomsInVerbalDirections == true, we list the axiom number at the
// beginning of each verbal direction.
bool ReferenceFinder::sAxiomsInVerbalDirections = true;

// Variables used when we calculate statistics on the database
int ReferenceFinder::sNumBuckets = 11;          // how many error buckets to use
double ReferenceFinder::sBucketSize = 0.001;    // size of each bucket
int ReferenceFinder::sNumTrials = 1000;         // number of test cases total
string ReferenceFinder::sStatistics;            // holds results of analysis
    
// Letters that are used for labels for marks and lines.
char RefLine::sLabels[] = "ABCDEFGHIJ";
char RefMark::sLabels[] = "PQRSTUVWXYZ";


/*****
private static member initialization
*****/
RefContainer<RefLine> ReferenceFinder::sBasisLines;
RefContainer<RefMark> ReferenceFinder::sBasisMarks;
ReferenceFinder::DatabaseFn ReferenceFinder::sDatabaseFn = 0;
void* ReferenceFinder::sDatabaseUserData = 0;
ReferenceFinder::StatisticsFn ReferenceFinder::sStatisticsFn = 0;
void* ReferenceFinder::sStatisticsUserData = 0;
int ReferenceFinder::sStatusCount = 0;
ReferenceFinder::rank_t ReferenceFinder::sCurRank = 0;


#ifdef __MWERKS__
#pragma mark -
#endif


/*****
Routine called by RefContainer<R> to report progress during the time-consuming
process of initialization. This routine updates our private counter each time
it's called and only occasionally passes on a full call to sDatabaseFn. Clients
can adjust the frequency of calling by changing the variable
sDatabaseStatusSkip. If the client DatabaseFn sets the value of haltFlag to
true, we immediately terminate construction of references.
*****/
void ReferenceFinder::CheckDatabaseStatus()
{
  if (sStatusCount < sDatabaseStatusSkip) sStatusCount++;
  else {
    bool haltFlag = false;
    if (sDatabaseFn) (*sDatabaseFn)(
      DatabaseInfo(DATABASE_WORKING, sCurRank, GetNumLines(), GetNumMarks()), 
      sDatabaseUserData, haltFlag);
    if (haltFlag) throw EXC_HALT();
    sStatusCount = 0;
  }
}


/*****
Create all marks and lines of a given rank.
*****/
void ReferenceFinder::MakeAllMarksAndLinesOfRank(rank_t arank)
{
  sCurRank = arank;
  
  // Construct all types of lines of the given rank. Note that the order in
  // which we call the MakeAll() functions determines which types of RefLine
  // get built, since the first object with a given key to be constructed gets
  // the key slot.

  // We give first preference to lines that don't involve making creases
  // through points, because these are the hardest to do accurately in practice.
  if (sUseRefLine_L2L) RefLine_L2L::MakeAll(arank);
  if (sUseRefLine_P2P) RefLine_P2P::MakeAll(arank);
  if (sUseRefLine_L2L_P2L) RefLine_L2L_P2L::MakeAll(arank);
  if (sUseRefLine_P2L_P2L) RefLine_P2L_P2L::MakeAll(arank);
  
  // Next, we'll make lines that put a crease through a single point.
  if (sUseRefLine_P2L_C2P) RefLine_P2L_C2P::MakeAll(arank);
  if (sUseRefLine_L2L_C2P) RefLine_L2L_C2P::MakeAll(arank);
    
  // Finally, we'll do lines that put a crease through both points. 
  if (sUseRefLine_C2P_C2P) RefLine_C2P_C2P::MakeAll(arank);
      
  // Having constructed all lines in the buffer, add them to the main collection.
  sBasisLines.FlushBuffer();
  
  // construct all types of marks of the given rank
  RefMark_Intersection::MakeAll(arank);
  sBasisMarks.FlushBuffer();
  
  // if we're reporting status, say how many we constructed.
  bool haltFlag = false;
  if (sDatabaseFn) (*sDatabaseFn)(
    DatabaseInfo(DATABASE_RANK_COMPLETE, arank, GetNumLines(), GetNumMarks()), 
    sDatabaseUserData, haltFlag);
  if (haltFlag) throw EXC_HALT();
}


/*****
Create all marks and lines sequentially. you should have previously verified
that LineKeySizeOK() and MarkKeySizeOK() return true.
*****/
void ReferenceFinder::MakeAllMarksAndLines()
{
  // Start by clearing out any old marks or lines; this is so we can restart if
  // we want.
  sBasisLines.Rebuild();
  sBasisMarks.Rebuild();
  
  // Let the user know that we're initializing and what operations we're using.
  bool haltFlag = false;
  if (sDatabaseFn) (*sDatabaseFn)(
    DatabaseInfo(DATABASE_INITIALIZING, 0, GetNumLines(), GetNumMarks()),
    sDatabaseUserData, haltFlag);
  
  // Build a bunch of marks of successively higher rank. Note that building
  // lines up to rank 4 and marks up to rank 8 with no limits would result in
  // 4185 lines and 1,090,203 marks, which would take about 60 MB of memory.
  
  // Rank 0: Construct the four edges of the square.
  sBasisLines.Add(new RefLine_Original(sPaper.mBottomEdge, 0, 
    string("the bottom edge")));
  sBasisLines.Add(new RefLine_Original(sPaper.mLeftEdge, 0, 
    string("the left edge")));
  sBasisLines.Add(new RefLine_Original(sPaper.mRightEdge, 0, 
    string("the right edge")));
  sBasisLines.Add(new RefLine_Original(sPaper.mTopEdge, 0, 
    string("the top edge")));
    
  // Rank 0: Construct the four corners of the square.
  sBasisMarks.Add(new RefMark_Original(sPaper.mBotLeft, 0, 
    string("the bottom left corner")));
  sBasisMarks.Add(new RefMark_Original(sPaper.mBotRight, 0, 
    string("the bottom right corner")));
  sBasisMarks.Add(new RefMark_Original(sPaper.mTopLeft, 0, 
    string("the top left corner")));
  sBasisMarks.Add(new RefMark_Original(sPaper.mTopRight, 0, 
    string("the top right corner")));
    
  // Report our status for rank 0.
  if (sDatabaseFn) (*sDatabaseFn)(
    DatabaseInfo(DATABASE_RANK_COMPLETE, 0, GetNumLines(), GetNumMarks()), 
    sDatabaseUserData, haltFlag);
  
  // Rank 1: Construct the two diagonals.
  sBasisLines.Add(new RefLine_Original(sPaper.mUpwardDiagonal, 1, 
    string("the upward diagonal")));
  sBasisLines.Add(new RefLine_Original(sPaper.mDownwardDiagonal, 1, 
    string("the downward diagonal")));
    
  // Flush the buffers.
  sBasisLines.FlushBuffer();
  sBasisMarks.FlushBuffer();

  // Now build the rest, one rank at a time, starting with rank 1. This can
  // be terminated by a EXC_HALT if the user cancelled during the callback.
  try {
    for (rank_t irank = 1; irank <= sMaxRank; irank++) {
      MakeAllMarksAndLinesOfRank(irank);
    }
  }
  catch(EXC_HALT) {
    sBasisLines.FlushBuffer();
    sBasisMarks.FlushBuffer();
  }

  // Once that's done, all the objects are in the sortable arrays and we can
  // free up the memory used by the maps.
  sBasisLines.ClearMaps();
  sBasisMarks.ClearMaps();
  
  // And perform a final update of progress.
  if (sDatabaseFn) (*sDatabaseFn)(
    DatabaseInfo(DATABASE_READY, sCurRank, GetNumLines(), GetNumMarks()), 
    sDatabaseUserData, haltFlag);
}


/*****
Find the best marks closest to a given point ap, storing the results in the
vector vm.
*****/
void ReferenceFinder::FindBestMarks(const XYPt& ap, vector<RefMark*>& vm, 
  short numMarks)
{
  vm.resize(numMarks);
  partial_sort_copy(sBasisMarks.begin(), sBasisMarks.end(), vm.begin(), vm.end(), 
    CompareRankAndError<RefMark>(ap));
}


/*****
Find the best lines closest to a given line al, storing the results in the
vector vl.
*****/
void ReferenceFinder::FindBestLines(const XYLine& al, vector<RefLine*>& vl, 
  short numLines)
{
  vl.resize(numLines);
  partial_sort_copy(sBasisLines.begin(), sBasisLines.end(), vl.begin(), vl.end(), 
    CompareRankAndError<RefLine>(al));
}


/*****
Return true if ap is a valid mark. Return an error message if it isn't.
*****/
bool ReferenceFinder::ValidateMark(const XYPt& ap, string& err)
{
  if (ap.x < 0 || ap.x > sPaper.mWidth) {
    stringstream ss;
    ss << "Error -- x coordinate should lie between 0 and " << 
      sPaper.mWidth;
    err = ss.str();
    return false;
  }
  
  if (ap.y < 0 || ap.y > sPaper.mHeight) {
    stringstream ss;
    ss << "Error -- y coordinate should lie between 0 and " << 
      sPaper.mHeight;
    err = ss.str();
    return false;
  }
  return true;
}


/*****
Validate the two entered points that define the line. Return an error message
if they aren't distinct.
*****/
bool ReferenceFinder::ValidateLine(const XYPt& ap1, const XYPt& ap2, 
  string& err)
{
  if ((ap1 - ap2).Mag() > EPS) return true;
  stringstream ss;
  ss.precision(10);
  ss << "Error -- the two points must be distinct (separated by at least " <<
      EPS << ").";
  err = ss.str();
  return false;
}


/*****
Compute statistics on the accuracy of the current set of marks for a randomly 
chosen set of points and pass the results in our static string variable.
*****/
void ReferenceFinder::CalcStatistics()
{
  bool cancel = false;
  if (sStatisticsFn) {
    sStatisticsFn(StatisticsInfo(STATISTICS_BEGIN), 
      sStatisticsUserData, cancel);
  }
  
  vector<int> errBucket;              // number of errors in each bucket
  errBucket.assign(sNumBuckets, 0);
  vector<double> errors;              // list of all errors
  vector <RefMark*> sortMarks(1);     // a vector to do our sorting into
  
  // Run a bunch of test cases on random points.
  int actNumTrials = sNumTrials;
  for (size_t i = 0; i < size_t(sNumTrials); i++) {
    XYPt testPt((double(rand()) / (RAND_MAX * sPaper.mWidth)), 
      double(rand()) / (RAND_MAX * sPaper.mHeight));
    
    // Find the mark closest to the test mark.
    partial_sort_copy(sBasisMarks.begin(), sBasisMarks.end(), 
      sortMarks.begin(), sortMarks.end(), CompareError<RefMark>(testPt));
      
    // note how close we were
    double error = (testPt - sortMarks[0]->p).Mag();
    errors.push_back(error);
    // Report progress, and check for early termination from user
    if (sStatisticsFn) {
      sStatisticsFn(StatisticsInfo(STATISTICS_WORKING, i, error), 
        sStatisticsUserData, cancel);
      if (cancel) {
        actNumTrials = 1 + int(i);
        break;
      }
    }
    
    // Compute a bucket index for this error. Over the top goes into last
    // bucket. Then record the error in the appropriate bucket.
    int errindex = int(error / sBucketSize);
    if (errindex >= sNumBuckets) errindex = sNumBuckets - 1;
    errBucket[errindex] += 1;
  }
  
  // Now compose a report of the results.
  stringstream ss;
  ss << fixed << showpoint << setprecision(1);
  ss << "Distribution of errors for " << actNumTrials << " trials:" << endl;
  int total = 0;
  // Report the number of errors for each error bucket
  for (int i = 0; i < sNumBuckets - 1; i++) {
    total += errBucket[i];
    ss << "error < " << 
      setprecision(3) << sBucketSize * (i + 1) << 
      " = " << total << " (" << 
      setprecision(1) << 100. * double(total) / actNumTrials << 
      "%)" << endl;
  }
  ss << "error > " << 
    setprecision(3) << sBucketSize * (sNumBuckets - 1) << 
    " = " << (actNumTrials - total) << " (" << 
    setprecision(1) << 100. * double(actNumTrials - total) / actNumTrials << 
    "%)" << endl;
    
  // Sort the errors and write percentiles of the errors into output string
  sort(errors.begin(), errors.end());
  ss << setprecision(4);
  ss << endl << "Distribution of errors:" << endl;
  ss << "10th percentile :" << errors[int(.10 * errors.size())] << endl;
  ss << "20th percentile :" << errors[int(.20 * errors.size())] << endl;
  ss << "50th percentile :" << errors[int(.50 * errors.size())] << endl;
  ss << "80th percentile :" << errors[int(.80 * errors.size())] << endl;
  ss << "90th percentile :" << errors[int(.90 * errors.size())] << endl;
  ss << "95th percentile :" << errors[int(.95 * errors.size())] << endl;
  ss << "99th percentile :" << errors[int(.99 * errors.size())] << endl;
  
  sStatistics = ss.str();
  
  // Call the callback for the final time, passing the string containing the
  // results.
  if (sStatisticsFn) {
    sStatisticsFn(StatisticsInfo(STATISTICS_DONE), 
      sStatisticsUserData, cancel);
  }
}


/*****
This routine builds Peter Messer's construction of  cube root of 2. Only used
for testing, but I'll leave it in here for edification.
*****/
void ReferenceFinder::MesserCubeRoot(ostream& os)
{
  // Rank 0: Construct the four edges of the square.
  RefLine *be, *le, *re, *te;
  
  ReferenceFinder::sBasisLines.Add(be = new RefLine_Original(
    sPaper.mBottomEdge, 0, string("bottom edge")));
  ReferenceFinder::sBasisLines.Add(le = new RefLine_Original(
    sPaper.mLeftEdge, 0, string("left edge")));
  ReferenceFinder::sBasisLines.Add(re = new RefLine_Original(
    sPaper.mRightEdge, 0, string("right edge")));
  ReferenceFinder::sBasisLines.Add(te = new RefLine_Original
    (sPaper.mTopEdge, 0, string("top edge")));

  // Rank 0: Construct the four corners of the square.
  RefMark *blc, *brc, *tlc, *trc;
  
  ReferenceFinder::sBasisMarks.Add(blc = new RefMark_Original(
  sPaper.mBotLeft, 0, string("bot left corner")));
  ReferenceFinder::sBasisMarks.Add(brc = new RefMark_Original(
  sPaper.mBotRight, 0, string("bot right corner")));
  ReferenceFinder::sBasisMarks.Add(tlc = new RefMark_Original(
  sPaper.mTopLeft, 0, string("top left corner")));
  ReferenceFinder::sBasisMarks.Add(trc = new RefMark_Original(
  sPaper.mTopRight, 0, string("top right corner")));

  // Create the endpoints of the two initial fold lines
  RefMark *rma1, *rma2, *rmb1, *rmb2;
  ReferenceFinder::sBasisMarks.Add(rma1 = new RefMark_Original(XYPt(0, 1./3), 0, 
    string("(0, 1/3)")));
  ReferenceFinder::sBasisMarks.Add(rma2 = new RefMark_Original(XYPt(1, 1./3), 0, 
    string("(1, 1/3)")));
  ReferenceFinder::sBasisMarks.Add(rmb1 = new RefMark_Original(XYPt(0, 2./3), 0, 
    string("(0, 2/3)")));
  ReferenceFinder::sBasisMarks.Add(rmb2 = new RefMark_Original(XYPt(1, 2./3), 0, 
    string("(1, 2/3)")));

  // Create and add the two initial fold lines.
  RefLine *rla, *rlb;
  ReferenceFinder::sBasisLines.Add(rla = new RefLine_C2P_C2P(rma1, rma2));
  ReferenceFinder::sBasisLines.Add(rlb = new RefLine_C2P_C2P(rmb1, rmb2));
  
  // Construct the fold line
  RefLine_P2L_P2L rlc(brc, le, rma2, rlb, 0);
  
  // Print the entire sequence
  rlc.PutHowtoSequence(os);
  
  // quit the program
  exit(1);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 1: lightweight classes that represent points and lines
******************************************************************************/

/**********
class XYPt - a 2-vector that represents a point or 2-vector (direction vector).
It is a lightweight mathematical object. Most operations are defined in the
header, intended to be inlined for speed.
**********/

/*****
Stream I/O for a XYPt spits out the point as a parenthesis-enclosed,
comma-delimited pair. 
*****/
ostream& operator<<(ostream& os, const XYPt& p)
{
  return os << "(" << (p.x) << "," << (p.y) << ")";
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class XYLine - a class for representing a line by a scalar d and the unit
normal to the line, u, where the point d*u is the point on the line closest to
the origin. Note that (d,u) and (-d,-u) represent the same line. If u isn't a
unit vector, many things will break, so when in doubt, use Normalize() or
NormalizeSelf().
**********/

/*****
Stream I/O for a XYLine spits out the values of d and u as a
parenthesis-enclosed, comma-delimited pair, e.g., "(0,(1,0))".
*****/
ostream& operator<<(ostream& os, const XYLine& l)
{
  return os << "(" << (l.d) << "," << (l.u) << ")";
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class XYRect - a class for representing rectangles by two points, the bottom
left and top right corners.
**********/

/*****
Return the aspect ratio of the rectangle, defined to be the smaller dimension
divided by the larger dimension. If it's empty, return zero. If the rectangles
is improperly defined, the aspect ratio may be negative.
*****/
double XYRect::GetAspectRatio() const
{
  double wd = GetWidth();
  double ht = GetHeight();
  if (abs(wd) < EPS && abs(ht) < EPS) return 0;
  if (abs(wd) <= abs(ht)) return wd / ht;
  return ht / wd;
}


/*****
Stream I/O for a XYRect spits out the values of bl and tr as a
parenthesis-enclosed, comma-delimited pair, e.g., "((0,0),(1,1))".
*****/
ostream& operator<<(ostream& os, const XYRect& r)
{
  return os << "(" << (r.bl) << "," << (r.tr) << ")";
}


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 2: classes that represent reference marks and lines on a piece of paper
******************************************************************************/

/**********
class Paper - specialization of XYRect, used for representing the paper
**********/

/*****
Constructor
*****/
Paper::Paper(double aWidth, double aHeight) :
  XYRect(aWidth, aHeight)
{
  SetSize(aWidth, aHeight);
}


/*****
Set the size of the paper and all of the incidental data
*****/
void Paper::SetSize(double aWidth, double aHeight)
{
  bl = XYPt(0, 0);
  tr = XYPt(aWidth, aHeight);
  mWidth= aWidth;
  mHeight = aHeight;
  mBotLeft = XYPt(0, 0);
  mBotRight = XYPt(aWidth, 0);
  mTopLeft = XYPt(0, aHeight);
  mTopRight = XYPt(aWidth, aHeight);
  mTopEdge = XYLine(mTopLeft, mTopRight);
  mLeftEdge = XYLine(mBotLeft, mTopLeft);
  mRightEdge = XYLine(mBotRight, mTopRight);
  mBottomEdge = XYLine(mBotLeft, mBotRight);
  mUpwardDiagonal = XYLine(mBotLeft, mTopRight);
  mDownwardDiagonal = XYLine(mTopLeft, mBotRight);
}


/*****
Clip the line al to the paper, returning the endpoints of the clipped segment
in ap1 and ap2. If the line misses the paper entirely, return false and leave
ap1 and ap2 unchanged.
*****/
bool Paper::ClipLine(const XYLine& al, XYPt& ap1, XYPt& ap2) const
{
  // Start by collecting all points of intersection between the line and the
  // four sides of the paper.
  
  unsigned npts = 0;      // counter for number of points of intersection
  XYPt ipts[4];       // list of points of intersection
  XYPt p;           // a scratch pad for intersection points
  
  if (mTopEdge.Intersects(al, p) && Encloses(p)) ipts[npts++] = p;
  if (mLeftEdge.Intersects(al, p) && Encloses(p)) ipts[npts++] = p;
  if (mRightEdge.Intersects(al, p) && Encloses(p)) ipts[npts++] = p;
  if (mBottomEdge.Intersects(al, p) && Encloses(p)) ipts[npts++] = p;
  
  if (npts == 0) return false;  // line entirely misses the paper

  // Now parameterize all four points along the line, recording the minimum
  // and maximum parameter values.
  
  XYPt pt = al.d * al.u;          // a point on the line
  XYPt up = al.u.Rotate90();        // a tangent to the line
  double tmin =( ipts[0] - pt).Dot(up);
  double tmax = tmin;
  for (unsigned i = 1; i < npts; i++) {
    double tt = (ipts[i] - pt).Dot(up);
    if (tmin < tt) tmin = tt;
    if (tmax > tt) tmax = tt;
  };
  
  // Compute the endpoints from our parameter ranges.
  
  ap1 = pt + tmin * up;
  ap2 = pt + tmax * up;
  return true;
}


/*****
Return true if line al overlaps the paper in its interior. Return false if the
line misses the paper entirely, only hits a corner, or only runs along an edge.
*****/
bool Paper::InteriorOverlaps(const XYLine& al) const
{
  XYPt pa, pb;                // endpoints of the fold line
  if (!ClipLine(al, pa, pb)) return false;  // the line completely misses the paper
  
  if ((pa - pb).Mag() < EPS) return false;  // line hits at a single point (a corner)
  
  if (!GetBoundingBox(pa, pb).IsEmpty()) return true;  // bounding box has positive volume
  
  // If still here, then the bounding box must be a line, either the edge of the paper
  // or a vertical or horizontal line in the interior. We can test for the latter by 
  // seeing if the midpoint of the line lies fully in the interior of the paper.
  
  XYPt mp = MidPoint(pa, pb);
  
  if (mTopEdge.Intersects(mp) || mBottomEdge.Intersects(mp) || 
    mLeftEdge.Intersects(mp) || mRightEdge.Intersects(mp)) return false;
  
  return true;
}


/*****
The line al divides the paper into two portions. Return true if either of the
two qualifies as a skinny flap. "Skinny" means a triangle (or quad) whose
aspect ratio falls below a minimum size.
*****/
bool Paper::MakesSkinnyFlap(const XYLine& al) const 
{
  // Since "true" = "bad", we'll return true for any failures along the way due to bad
  // input parameters.
  
  XYPt p1, p2;            // endpoints of the line al
  ClipLine(al, p1, p2);       // get the endpoints of the fold line on the paper
  
  XYLine lb;              // perpendicular bisector of line segment p1-p2
  lb.u = al.u.Rotate90();
  lb.d = MidPoint(p1, p2).Dot(lb.u);
  XYPt bp1, bp2;            // endpoints of the bisector
  ClipLine(lb, bp1, bp2);       // get the endpoints of the bisector
  
  // Get the bounding box that contains the fold line and a point on either side of the
  // fold line. If this bounding box is below the minimum aspect ratio, then it contains
  // a flap that falls below the minimum aspect ratio, so we return true.

  if (abs(GetBoundingBox(p1, p2, bp1).GetAspectRatio()) < ReferenceFinder::sMinAspectRatio) 
    return true;
  if (abs(GetBoundingBox(p1, p2, bp2).GetAspectRatio()) < ReferenceFinder::sMinAspectRatio) 
    return true;
  
  // If we're still here, we didn't create any skinny flaps, so we're cool.
  
  return false;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefBase - base class for a mark or line. 
**********/

/*
Non-static member variables are:
mRank - the rank, which is the number of creases that need to be made to define
  it.
mKey - a unique key that is used to efficiently store and search over marks and
  lines. mKey is initialized to 0. If the object has been successfully
  constructed, it will be set to an integer greater than 0, so mKey==0 is used
  as a test for successful construction.
mIndex - a counter used in constructing the verbal sequence; basically, it's
  the order in which the object is created for a given folding sequence.
Any subclasses of RefBase should be fairly lightweight because we'll be
creating a couple hundred thousand of them during program initialization.
*/

/*****
RefBase static member initialization
*****/
RefDgmr* RefBase::sDgmr;
vector<RefBase*> RefBase::sSequence;
vector<RefBase::DgmInfo> RefBase::sDgms;


/*  Notes on Sequences.
A ref (RefMark or RefLine) is typically defined in terms of other refs, going
all the way back to RefMark_Original and RefLine_Original. The routine
aRef->BuildAndNumberSequence() constructs an ordered list of all the refs that
make up aRef. The ordering is such that ancestor refs come earlier in the list
than refs derived from them. In addition to constructing the list,
BuildAndNumberSequence also sets the mIndex variable of each non- original ref,
so that each can be given a unique name (which depends on the index, and is a
letter, A-J for lines, P-Z for points).

This numbering scheme means that there can only be one sequence in existence at
a time. To insure this, the sequence is constructed in the static RefBase
member variable sSequence, which is public, so you can (if you like) read out
this sequence.
*/

/*****
Append to the end of sSequence pointers to all the marks and lines needed to
create this mark or line. Default behavior is to append a pointer to self. A
subclass line or mark made from others should call SequencePushUnique() for
each of the elements that define it, and then call the RefBase method for
itself.
*****/
void RefBase::SequencePushSelf()
{
  SequencePushUnique(this);
}


/*****
Build a sequence (in sSequence) of all of the references that are needed to
define this one; also set mIndex for each reference so that the relevant
RefMarks and RefLines are sequentially numbered.
*****/
void RefBase::BuildAndNumberSequence()
{
  sSequence.clear();
  SequencePushSelf(); 
  RefMark::ResetCount();
  RefLine::ResetCount();
  for (size_t i = 0; i < sSequence.size(); i++) sSequence[i]->SetIndex();
}


/*****
Put a statement about how to make this mark from its constituents to a stream.
Overridden by most subclasses. Return true if we actually put something
(original types will return false).
*****/
bool RefBase::PutHowto(ostream& /* os */) const
{
  return false;
}


/*****
Send the full how-to sequence to the given stream.
*****/
ostream& RefBase::PutHowtoSequence(ostream& os)
{
  BuildAndNumberSequence(); 
  for (size_t i = 0; i < sSequence.size(); i++)
    if (sSequence[i]->PutHowto(os)) os << "." << endl;
  return os;
}
    

/*  Notes on diagrams.
A DgmInfo is a very simple object that contains just a couple of bits of
information necessary to construct a complete diagram from the list of refs
contained in sSequence. Any diagram is a subsequence of sSequence, which, in
effect, describes all the refs that are already on the paper as well as the
ref(s) currently being made.

DgmInfo.idef is the index of the first ref in sSequence that is defined in the
current diagram.

DgmInfo.iact is the index of the last ref that is defined in the current
diagram, which is the action line, if this diagram includes an action line.
idef <= iact.
*/

/*****
Build a set of diagrams that describe how to fold this reference, by
constructing a list of DgmInfo records (which refer to elements and
subsequences of sSequence).
*****/
void RefBase::BuildDiagrams()
{
  sDgms.clear();
  BuildAndNumberSequence();
  
  // Now, we need to note which elements of the sequence are action lines;
  // there will be a diagram for each one of these.
  size_t ss = sSequence.size();
  for (size_t i = 0; i < ss; i++)
    if (sSequence[i]->IsActionLine()) sDgms.push_back(DgmInfo(i, i));
    
  // We should always have at least one diagram, even if there was only one ref
  // in sSequence (which happens if the ref was a RefMark_Original or
  // RefLine_Original).
  if (sDgms.size() == 0) sDgms.push_back(DgmInfo(0, 0));
  
  // And we make sure we have a diagram for the last ref in the sequence (which
  // might not be the case if we ended with a RefMark or an original).
  if (sDgms[sDgms.size() - 1].iact < ss - 1) sDgms.push_back(DgmInfo(0, ss - 1));
  
  // Now we go through and set the idef fields of each DgmInfo record.
  size_t id = 0;
  for (size_t i = 0; i < sDgms.size(); i++) {
    sDgms[i].idef = id;
    id = sDgms[i].iact + 1;
  }
}


/*****
Draw the paper
*****/
void RefBase::DrawPaper()
{
  vector<XYPt> corners;
  corners.push_back(ReferenceFinder::sPaper.mBotLeft);
  corners.push_back(ReferenceFinder::sPaper.mBotRight);
  corners.push_back(ReferenceFinder::sPaper.mTopRight);
  corners.push_back(ReferenceFinder::sPaper.mTopLeft);
  sDgmr->DrawPoly(corners, RefDgmr::POLYSTYLE_WHITE);
}


/*****
Draw the given diagram using the RefDgmr aDgmr.
*****/
void RefBase::DrawDiagram(RefDgmr& aDgmr, const DgmInfo& aDgm)
{
  // Set the current RefDgmr to be aDgmr.
  sDgmr = &aDgmr;
  
  // always draw the paper
  DrawPaper();
  
  // Make a note of the action line ref
  RefBase* ral = sSequence[aDgm.iact];
  
  // draw all refs specified by the DgmInfo. Most get drawn in normal style.
  // The ref that is the action line (and all subsequent refs) get drawn in
  // action style. Any refs that are used immediately by the action line get
  // drawn in hilite style. Drawing for each diagram is done in multiple passes
  // so that, for examples, labels end up on top of everything else.
  for (short ipass = 0; ipass < NUM_PASSES; ipass++) {
    for (size_t i = 0; i < aDgm.iact; i++) {
      RefBase* rb = sSequence[i];
      if ((i >= aDgm.idef && rb->IsDerived()) || ral->UsesImmediate(rb)) 
        rb->DrawSelf(REFSTYLE_HILITE, ipass);
      else rb->DrawSelf(REFSTYLE_NORMAL, ipass);
    };
    sSequence[aDgm.iact]->DrawSelf(REFSTYLE_ACTION, ipass);
  }
}


/*****
Put the caption to a particular diagram to a stream. The caption consists of
how-to for those refs that are part of the action. The output is created as a
single string containing possibly multiple sentences.
*****/
void RefBase::PutDiagramCaption(std::ostream& os, const DgmInfo& aDgm)
{
  for (size_t i = aDgm.idef; i <= aDgm.iact; i++) {
    sSequence[i]->PutHowto(os);
    os << ". ";
  }
}


/*****
Return true if this mark or line uses rb for immediate reference. "Immediate"
means that if A uses B and B uses C, A->UsesImmediate(B) returns true but
A->UsesImmediate(C) returns false. Default returns false; this will be used by
original marks and lines.
*****/
bool RefBase::UsesImmediate(RefBase* /* rb */) const
{
  return false;
}


/*****
Return true if this is a derived (rather than an original) mark or line. Note
that this is not the same as mRank!=0, because the diagonals have mRank==1 but
are still considered original. Default is true since most objects will be
derived.
*****/
bool RefBase::IsDerived() const
{
  return true;
}


/*****
Utility used by subclasses when they implement SequencePushSelf(). This insures
that a given mark only gets a single label.
*****/
void RefBase::SequencePushUnique(RefBase* rb)
{
  if (find(sSequence.begin(), sSequence.end(), rb) == sSequence.end()) 
    sSequence.push_back(rb);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefMark - base class for a mark on the paper. 
**********/

/*****
RefMark static member initialization
*****/
RefBase::index_t RefMark::sCount = 0;   // Initialize class index


/*****
Calculate the key value used for distinguishing RefMarks. This should be called
at the end of every constructor if the mark is valid (and not if it isn't).
*****/
void RefMark::FinishConstructor()
 {
  const double fx = p.x / ReferenceFinder::sPaper.mWidth;  // fx is between 0 and 1
  const double fy = p.y / ReferenceFinder::sPaper.mHeight; // fy is between 0 and 1

  key_t nx = static_cast<key_t> (floor(0.5 + fx * ReferenceFinder::sNumX));
  key_t ny = static_cast<key_t> (floor(0.5 + fy * ReferenceFinder::sNumY));
  mKey = 1 + nx * ReferenceFinder::sNumY + ny;
}


/*****
Return the distance to a point ap. This is used when sorting marks by their
distance from a given mark.
*****/
double RefMark::DistanceTo(const XYPt& ap) const
 {
  return (p - ap).Mag();
 }
 

/*****
Return true if this mark is on the edge of the paper
*****/
bool RefMark::IsOnEdge() const
{
  return (ReferenceFinder::sPaper.mLeftEdge.Intersects(p) || 
    ReferenceFinder::sPaper.mRightEdge.Intersects(p) ||
    ReferenceFinder::sPaper.mTopEdge.Intersects(p) || 
    ReferenceFinder::sPaper.mBottomEdge.Intersects(p));
}


/*****
Return false, since marks can never be actions
*****/
bool RefMark::IsActionLine() const
{
  return false;
}


/*****
Return the label for this mark.
*****/
const char RefMark::GetLabel() const
{
  return sLabels[mIndex - 1];
}


/*****
Put the name of this mark to a stream. Default behavior gives this mark a
letter. Return true if we used a letter, false if something else (i.e., the
name of a RefMark_Original).
*****/
bool RefMark::PutName(ostream& os) const
{
  os << "point " << GetLabel()
#ifdef RF_PUT_KEY_IN_TEXT
    << "[" << mKey << "]"
#endif // RF_PUT_KEY_IN_TEXT
  ;
  return true;
}


/*****
Put the distance from this mark to point ap to a stream.
*****/
void RefMark::PutDistanceAndRank(ostream& os, const XYPt& ap) const
{
  os.precision(4);
  os.setf(ios_base::fixed, ios_base::floatfield);
  os << "Solution " << p.Chop() << ": err = " << DistanceTo(ap) << " (rank " << 
    mRank << ") ";
}
    
    
/*****
Draw a RefMark in the indicated style
*****/
void RefMark::DrawSelf(RefStyle rstyle, short ipass) const
{
  switch(ipass) {
    case PASS_POINTS:
      {
        switch(rstyle) {
          case REFSTYLE_NORMAL:
            sDgmr->DrawPt(p, RefDgmr::POINTSTYLE_NORMAL);
            break;
          case REFSTYLE_HILITE:
            sDgmr->DrawPt(p, RefDgmr::POINTSTYLE_HILITE);
            break;
          case REFSTYLE_ACTION:
            sDgmr->DrawPt(p, RefDgmr::POINTSTYLE_ACTION);
            break;
        }
      };
      break;
      
    case PASS_LABELS:
      {
      string sm(1, GetLabel());
      switch(rstyle) {
        case REFSTYLE_NORMAL:
          // Normal points don't get labels drawn
          break;
        case REFSTYLE_HILITE:
          sDgmr->DrawLabel(p, sm, RefDgmr::LABELSTYLE_HILITE);
          break;
        case REFSTYLE_ACTION:
          sDgmr->DrawLabel(p, sm, RefDgmr::LABELSTYLE_ACTION);
          break;
      };
      break;
    }
  }
}


/*****
Most types of RefMark use the default method, which gives the mark an index
from the class variable sCount and then bumps up the class variable.
*****/
void RefMark::SetIndex()
{
  mIndex = ++sCount;
}


/*****
Reset the counter used for indexing marks in a sequence.
*****/
void RefMark::ResetCount()
{
  sCount = 0;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefMark_Original - Specialization of RefMark that represents a named mark
(e.g., a corner).
**********/

/*****
Constructor.
*****/
RefMark_Original::RefMark_Original(const XYPt& ap, rank_t arank, string aName) : 
  RefMark(ap, arank), mName(aName)
{
  FinishConstructor();
}


/*****
Return the label for this mark.
*****/
const char RefMark_Original::GetLabel() const
{
  return 0; // originals get no labels
}


/*****
Put the name of this mark to a stream. Override to put the string name (rather
than a letter).
*****/

bool RefMark_Original::PutName(ostream& os) const
{
  os << mName;  // return the string
  return false;
}


/*****
Draw this mark
*****/
void RefMark_Original::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Override the default because original marks don't get labels and are only
  // drawn when they are hilited or action (in which case we still draw them
  // hilited).
  if ((ipass == PASS_POINTS) && 
    (rstyle == REFSTYLE_HILITE || rstyle == REFSTYLE_ACTION)) 
    sDgmr->DrawPt(p, RefDgmr::POINTSTYLE_HILITE);
} 


/*****
Return false, since this is original
*****/
bool RefMark_Original::IsDerived() const
{
  return false;
}


/*****
Overridden because we don't use an index for named marks.
*****/
void RefMark_Original::SetIndex()
{
  mIndex = 0;
};


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefMark_Intersection - Specialization of a RefMark for a mark defined by
the intersection of 2 lines.
**********/

/*****
Constructor.
*****/
RefMark_Intersection::RefMark_Intersection(RefLine* arl1, RefLine* arl2) : 
  RefMark(CalcMarkRank(arl1, arl2)), rl1(arl1), rl2(arl2)
{
  // Get references to constituent math types
  
  const XYLine& l1 = rl1->l;
  const XYPt& u1 = rl1->l.u;
//  const double& d1 = rl1->l.d;

  const XYLine& l2 = rl2->l;
  const XYPt& u2 = rl2->l.u;
//  const double& d2 = rl2->l.d;  
  
  // If the lines don't intersect, it's not a valid point. If they do,
  // assign the intersection to the member variable p.
  
  if (!l1.Intersects(l2, p)) return;
  
  // If the intersection point falls outside the square, it's not valid.
  
  if (!ReferenceFinder::sPaper.Encloses(p)) return;
  
  // If the lines intersect at less than a 30 degree angle, we won't keep this 
  // point because such intersections are imprecise to use as reference points.
  
  if (abs(u1.Dot(u2.Rotate90())) < ReferenceFinder::sMinAngleSine) return;
    
  FinishConstructor();
}


/*****
Return true if this mark uses rb for immediate reference.
*****/
bool RefMark_Intersection::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rl2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefMark_Intersection::SequencePushSelf()
{
  rl1->SequencePushSelf();
  rl2->SequencePushSelf();
  RefBase::SequencePushSelf();
}


/*****
Put a description of how to construct this mark to the stream.
*****/
bool RefMark_Intersection::PutHowto(ostream& os) const
{
  os << "The intersection of ";
  rl1->PutName(os);
  os << " with ";
  rl2->PutName(os);
  os << " is ";
  PutName(os);
  if (ReferenceFinder::sClarifyVerbalAmbiguities) {
    os.precision(4);
    os.setf(ios_base::fixed, ios_base::floatfield);
    os << " = " << p.Chop();
  };
  return true;
}


/*****
Go through existing lines and create RefMark_Intersections with rank equal to
arank, up to a cumulative total of sMaxMarks.
*****/
void RefMark_Intersection::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= arank / 2; irank++) {
    rank_t jrank = arank - irank;
    bool sameRank = (irank == jrank);
    RefContainer<RefLine>::rank_iterator li = 
      ReferenceFinder::sBasisLines.maps[irank].begin();
    if (sameRank) li++;
    while (li != ReferenceFinder::sBasisLines.maps[irank].end()) {
      RefContainer<RefLine>::rank_iterator lj = 
        ReferenceFinder::sBasisLines.maps[jrank].begin();
      while (lj != (sameRank ? li : ReferenceFinder::sBasisLines.maps[jrank].end())) { 
        if (ReferenceFinder::GetNumMarks() >= ReferenceFinder::sMaxMarks) return;
        RefMark_Intersection rmi(li->second, lj->second);
        ReferenceFinder::sBasisMarks.AddCopyIfValidAndUnique(rmi);
        lj++;
      };
      li++;
    }
  } 
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine - base class for a reference line. 
**********/

/*****
RefLine static member initialization
*****/
RefBase::index_t RefLine::sCount = 0;   // Initialize class index


/*****
Calculate the key values used for sorting RefLines. Like its RefMark
counterpart, this should be called at the end of every successfully constructed
object, and it sets mKey to the proper value.
*****/
void RefLine::FinishConstructor()
{
  // resolve the ambiguity in line orientation by requiring d>=0.
  if (l.d < 0) {
    l.d = -l.d;
    l.u.x = -l.u.x;
    l.u.y = -l.u.y;
  };
  
  double fa = (1. + atan2(l.u.y, l.u.x) / (3.14159265358979323)) / 2.0; // fa is between 0 & 1
  const double dmax = sqrt(pow(ReferenceFinder::sPaper.mWidth, 2) + 
    pow(ReferenceFinder::sPaper.mHeight, 2));
  const double fd = l.d / dmax; // fd is between 0 and 1
  
  key_t nd = static_cast <key_t> (floor(0.5 + fd * ReferenceFinder::sNumD));
  if (nd == 0) fa = fmod(2 * fa, 1);  // for d=0, we map alpha and pi+alpha to the same key
  key_t na = static_cast <key_t> (floor(0.5 + fa * ReferenceFinder::sNumA));
  mKey = 1 + na * ReferenceFinder::sNumD + nd;
}


/*****
Return the "distance" between two lines.
*****/
double RefLine::DistanceTo(const XYLine& al) const
{
  if (ReferenceFinder::sLineWorstCaseError) {
    // Use the worst-case separation between the endpoints of the two lines
    // where they leave the paper.
    XYPt p1a, p1b, p2a, p2b;
    if (ReferenceFinder::sPaper.ClipLine(l, p1a, p1b) && 
      ReferenceFinder::sPaper.ClipLine(al, p2a, p2b)) {
      double err1 = max_val((p1a - p2a).Mag(), (p1b - p2b).Mag());
      double err2 = max_val((p1a - p2b).Mag(), (p1b - p2a).Mag());
      return min_val(err1, err2);
    }
    else {
      return 1 / EPS; // lines don't intersect the paper, return very large number
    }
  }
  else {
    // Use the Pythagorean sum of the distance between the characteristic
    // vectors of the tangent point and angle.
    return sqrt(pow(l.u.Dot(al.u.Rotate90()), 2) + 
      pow(l.d - al.d * l.u.Dot(al.u), 2));
  }
}


/*****
Return true if this RefLine is on the edge of the paper
*****/
bool RefLine::IsOnEdge() const
{
  return ((ReferenceFinder::sPaper.mLeftEdge == l) || 
    (ReferenceFinder::sPaper.mTopEdge == l) ||
    (ReferenceFinder::sPaper.mRightEdge == l) || 
    (ReferenceFinder::sPaper.mBottomEdge == l));
}


/*****
Return true, since MOST Reflines are actions
*****/
bool RefLine::IsActionLine() const
{
  return true;
}


/*****
Return the label for this line.
*****/
const char RefLine::GetLabel() const
{
  return sLabels[mIndex - 1];
}


/*****
Put the name of this line to a stream. Default behavior gives this line a
letter. Return true if we used a letter. (We'll return false if we use
something else, i.e., a RefLine_Original).
*****/
bool RefLine::PutName(ostream& os) const
{
  os << "line " << GetLabel()
#ifdef RF_PUT_KEY_IN_TEXT
    << "[" << mKey << "]"
#endif // RF_PUT_KEY_IN_TEXT
  ;
  return true;
}


/*****
Put the distance between this line and line al to a stream along with the rank
*****/
void RefLine::PutDistanceAndRank(ostream& os, const XYLine& al) const
{
  os.precision(4);
  os.setf(ios_base::fixed, ios_base::floatfield);
  os << "Solution " << l << ": err = " << DistanceTo(al) << " (rank " << 
    mRank << ") ";
}
    
    
/*****
Draw a line in the given style.
*****/
void RefLine::DrawSelf(RefStyle rstyle, short ipass) const
{
  XYPt p1, p2;
  ReferenceFinder::sPaper.ClipLine(l, p1, p2);
  
  switch(ipass) {
    case PASS_LINES:
      {
        switch (rstyle) {
          case REFSTYLE_NORMAL:
            sDgmr->DrawLine(p1, p2, RefDgmr::LINESTYLE_CREASE);
            break;
          default: ; // keep compiler happy
        }
      };
      break;
      
    case PASS_HLINES: // hilited lines and action lines go on top of others
      {
        switch (rstyle) {
          case REFSTYLE_HILITE:
            sDgmr->DrawLine(p1, p2, RefDgmr::LINESTYLE_HILITE);
            break;
          case REFSTYLE_ACTION:
            sDgmr->DrawLine(p1, p2, RefDgmr::LINESTYLE_VALLEY);
            break;
          default: ; // keep compiler happy
        }
      };
      break;
      
    case PASS_LABELS:
      {
        XYPt mp = MidPoint(p1, p2); // label goes at the midpoint of the line
        string sl(1, GetLabel());
        switch (rstyle) {
          case REFSTYLE_NORMAL:
            // normal lines don't get labels
            break;
          case REFSTYLE_HILITE:
            sDgmr->DrawLabel(mp, sl, RefDgmr::LABELSTYLE_HILITE);
            break;
          case REFSTYLE_ACTION:
            sDgmr->DrawLabel(mp, sl, RefDgmr::LABELSTYLE_ACTION);
            break;
          default: ;// keep compiler happy
        }
      };
      break;
    default: ;// keep compiler happy
  }
  // Subclasses will add arrows for REFSTYLE_ACTION
}


/*****
Most subclasses will use the default method, which sets the index from the
class variable sCount and then bumps up the count.
*****/

void RefLine::SetIndex()
{
  mIndex = ++sCount;
}


/*****
Reset the class variable sCount to zero.
*****/
void RefLine::ResetCount()
{
  sCount = 0;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_Original - Specialization of RefLine that represents a line that
is the edge of the paper or an initial crease (like the diagonal).
**********/

/*****
Constructor.
*****/
RefLine_Original::RefLine_Original(const XYLine& al, rank_t arank, string aName) : 
  RefLine(al, arank), mName(aName)
{
  FinishConstructor();
}


/*****
Return false because RefLine_Originals aren't actions, they're present from the
beginning.
*****/
bool RefLine_Original::IsActionLine() const
{
  return false;
}


/*****
Return the label for this line.
*****/
const char RefLine_Original::GetLabel() const
{
  return 0; // originals get no label
}


/*****
Put the name to a stream. Override the default method to use the actual string
name, rather than a letter. Return false since we didn't use a letter.
*****/

bool RefLine_Original::PutName(ostream& os) const
{
  os << mName;
  return false;
}


/*****
Draw this line in the appropriate style
*****/
void RefLine_Original::DrawSelf(RefStyle rstyle, short ipass) const
{
  // RefLine_Originals don't get labels, and they are REFSTYLE_ACTION, we
  // still draw them hilited.
  XYPt p1, p2;
  ReferenceFinder::sPaper.ClipLine(l, p1, p2);
  switch(ipass) {
    case PASS_LINES:
      switch (rstyle) {
        case REFSTYLE_NORMAL:
          sDgmr->DrawLine(p1, p2, RefDgmr::LINESTYLE_CREASE);
          break;
        default: ; // keep compiler happy
      }
      break;
    case PASS_HLINES:
      switch (rstyle) {
        case REFSTYLE_HILITE:
        case REFSTYLE_ACTION:
          sDgmr->DrawLine(p1, p2, RefDgmr::LINESTYLE_HILITE);
          break;
        default: ; // keep compiler happy
      }
      break;
    default: ; // keep compiler happy about unhandled cases
  }
}


/*****
Return false, since this is original
*****/
bool RefLine_Original::IsDerived() const
{
  return false;
}


/*****
Set the index of this line. Override the default method to use an index of
zero, since we've already got a name.
*****/

void RefLine_Original::SetIndex()
{
  mIndex = 0;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_C2P_C2P - Huzita-Hatori Axiom O1
Make a crease through two points p1 and p2.
**********/

/*****
Constructor. Initialize with the two marks that this line connects.
*****/
RefLine_C2P_C2P::RefLine_C2P_C2P(RefMark* arm1, RefMark* arm2) : 
  RefLine(CalcLineRank(arm1, arm2)), rm1(arm1), rm2(arm2)
{
  const XYPt& p1 = rm1->p;
  const XYPt& p2 = rm2->p;
  
  // Construct member data
  l.u = (p2 - p1).Rotate90().Normalize();
  l.d = .5 * (p1 + p2).Dot(l.u);
  
  // Don't need to check visibility because this type is always visible.
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;
  
  // This type is always valid.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_C2P_C2P::UsesImmediate(RefBase* rb) const
{
  return (rb == rm1 || rb == rm2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_C2P_C2P::SequencePushSelf()
{
  rm1->SequencePushSelf();
  rm2->SequencePushSelf();
  RefBase::SequencePushSelf();
}


/*****
Put the construction of this line to a stream.
*****/
bool RefLine_C2P_C2P::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[01] ";
  os << "Form a crease connecting ";
  rm1->PutName(os);
  os << " with ";
  rm2->PutName(os);
  os << ", making ";
  PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_C2P_C2P::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call the inherited method to draw the line
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need arrows.
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
  
    // Get the endpoints of the fold
    const XYPt& p1 = rm1->p;
    const XYPt& p2 = rm2->p;
    
    // Get the perpendicular bisector of the fold
    XYPt mp = MidPoint(p1, p2);
    XYLine lb;
    lb.u = l.u.Rotate90();
    lb.d = mp.Dot(lb.u);
    
    // Get the points where the bisector crosses the paper
    XYPt p3, p4;
    ReferenceFinder::sPaper.ClipLine(lb, p3, p4);
    
    // Parameterize these points along the bisector. Don't care about sign.
    double t3 = abs((p3 - mp).Dot(l.u));
    double t4 = abs((p4 - mp).Dot(l.u));
    
    // Construct a new pair of points that mate when folded and that are
    // guaranteed to lie within the paper.
    XYPt dp;
    if (t3 < t4) dp = t3 * l.u;
    else dp = t4 * l.u;
    p3 = mp + dp;
    p4 = mp - dp;
    
    // Draw an arrow that connects these two points.
    sDgmr->DrawFoldAndUnfoldArrow(p3, p4);
  }
}


/*****
Go through existing lines and marks and create RefLine_C2P_C2Ps with rank equal
to arank, up to a cumulative total of sMaxLines.
*****/
void RefLine_C2P_C2P::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1) / 2; irank++) {
    rank_t jrank = arank - irank - 1;
    bool sameRank = (irank == jrank);
    RefContainer<RefMark>::rank_iterator mi = 
      ReferenceFinder::sBasisMarks.maps[irank].begin();
    if (sameRank) mi++;
    while (mi != ReferenceFinder::sBasisMarks.maps[irank].end()) {
      RefContainer<RefMark>::rank_iterator mj = 
        ReferenceFinder::sBasisMarks.maps[jrank].begin();
      while (mj != (sameRank ? mi : ReferenceFinder::sBasisMarks.maps[jrank].end())) {
        if (ReferenceFinder::GetNumLines() >= ReferenceFinder::sMaxLines) return;
        RefLine_C2P_C2P rlc(mi->second, mj->second);
        ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rlc);
        mj++;
      };
      mi++;
    }
  }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_P2P - Huzita-Hatori Axiom O2
Bring p1 to p2.
**********/

/*****
Constructor.
*****/
RefLine_P2P::RefLine_P2P(RefMark* arm1, RefMark* arm2) : 
  RefLine(CalcLineRank(arm1, arm2)), rm1(arm1), rm2(arm2)
{
  // Get references to points
  XYPt& p1 = rm1->p;
  XYPt& p2 = rm2->p;
  
  // Construct member data
  l.u = (p2 - p1).Normalize();
  l.d = .5 * (p1 + p2).Dot(l.u);
  
  // Check visibility
  bool p1edge = arm1->IsOnEdge();
  bool p2edge = arm2->IsOnEdge();
  
  if (ReferenceFinder::sVisibilityMatters) {
    if (p1edge) mWhoMoves = WHOMOVES_P1;
    else if (p2edge) mWhoMoves = WHOMOVES_P2;
    else return;
  }
  else {
    mWhoMoves = WHOMOVES_P1;
  };
  
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;
  
  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_P2P::UsesImmediate(RefBase* rb) const
{
  return (rb == rm1 || rb == rm2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_P2P::SequencePushSelf()
{
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rm2->SequencePushSelf();
      rm1->SequencePushSelf();
      break;
    
    case WHOMOVES_P2:
      rm1->SequencePushSelf();
      rm2->SequencePushSelf();
      break;
  };    
  RefBase::SequencePushSelf();
}


/*****
Put the construction of this line to a stream.
*****/
bool RefLine_P2P::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[02] ";
  os << "Bring ";
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rm1->PutName(os);
      os << " to ";
      rm2->PutName(os);
      break;
      
    case WHOMOVES_P2:
      rm2->PutName(os);
      os << " to ";
      rm1->PutName(os);
      break;
  };
  os << ", making ";
  PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_P2P::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
    XYPt& p1 = rm1->p;
    XYPt& p2 = rm2->p;
    switch (mWhoMoves) {
      case WHOMOVES_P1:
        sDgmr->DrawFoldAndUnfoldArrow(p1, p2);
        break;
      case WHOMOVES_P2:
        sDgmr->DrawFoldAndUnfoldArrow(p2, p1);
        break;
    }
  }
}
      


/*****
Go through existing lines and marks and create RefLine_P2Ps with rank equal to
arank, up to a cumulative total of sMaxLines.
*****/
void RefLine_P2P::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1) / 2; irank++) {
    rank_t jrank = arank - irank - 1;
    bool sameRank = (irank == jrank);
    RefContainer<RefMark>::rank_iterator mi = 
      ReferenceFinder::sBasisMarks.maps[irank].begin();
    if (sameRank) mi++;
    while (mi != ReferenceFinder::sBasisMarks.maps[irank].end()) {
      RefContainer<RefMark>::rank_iterator mj = 
        ReferenceFinder::sBasisMarks.maps[jrank].begin();
      while (mj != (sameRank ? mi : ReferenceFinder::sBasisMarks.maps[jrank].end())) {
        if (ReferenceFinder::GetNumLines() >= ReferenceFinder::sMaxLines) return;
        RefLine_P2P rlb(mi->second, mj->second);
        ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rlb);
        mj++;
      };
      mi++;
    }
  }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_L2L - Huzita-Hatori Axiom O3 
Bring line l1 to line l2.
**********/

/*****
Constructor. iroot = 0 or 1.
*****/

RefLine_L2L::RefLine_L2L(RefLine* arl1, RefLine* arl2, short iroot) : 
  RefLine(CalcLineRank(arl1, arl2)), rl1(arl1), rl2(arl2)
{     
  // Get references to lines
  XYLine& l1 = rl1->l;
  XYPt& u1 = l1.u;
  double& d1 = l1.d;
  XYLine& l2 = rl2->l;
  XYPt& u2 = l2.u;
  double& d2 = l2.d;
  
  // Parallel lines get handled specially. There's only one solution; we arbitrarily make
  // it the iroot=0 solution.
  if (l1.IsParallelTo(l2)) {
    if (iroot == 0) {
      l.u = u1;
      l.d = .5 * (d1 + d2 * u2.Dot(u1));
    }
    else return; // iroot = 1 for parallel lines isn't a valid solution.
  }
  else {  // nonparallel lines
  
    // Construct the direction vector for the bisector, depending on the value of iroot.
    if (iroot == 0) l.u = (u1 + u2).Normalize();
    else l.u = (u1 - u2).Normalize();
    
    l.d = Intersection(l1, l2).Dot(l.u);
  };
  
  // If the paper doesn't overlap the fold line, we're not valid.
  if (!ReferenceFinder::sPaper.InteriorOverlaps(l)) return;
  
  // Check visibility
  bool l1edge = arl1->IsOnEdge();
  bool l2edge = arl2->IsOnEdge();
  
  if (ReferenceFinder::sVisibilityMatters) {
    if (l1edge) mWhoMoves = WHOMOVES_L1;
    else if (l2edge) mWhoMoves = WHOMOVES_L2;
    else {
      XYPt lp1, lp2;
      ReferenceFinder::sPaper.ClipLine(l1, lp1, lp2);
      if (ReferenceFinder::sPaper.Encloses(l.Fold(lp1)) && 
        ReferenceFinder::sPaper.Encloses(l.Fold(lp2))) mWhoMoves = WHOMOVES_L1;
      else {
        ReferenceFinder::sPaper.ClipLine(l2, lp1, lp2);
        if (ReferenceFinder::sPaper.Encloses(l.Fold(lp1)) && 
          ReferenceFinder::sPaper.Encloses(l.Fold(lp2))) mWhoMoves = WHOMOVES_L2;
        else return;
      }
    }
  }
  else {
    mWhoMoves = WHOMOVES_L1;
  };
  
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;

  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_L2L::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rl2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_L2L::SequencePushSelf()
{
  switch (mWhoMoves) {
    case WHOMOVES_L1:
      rl2->SequencePushSelf();
      rl1->SequencePushSelf();
      break;
    
    case WHOMOVES_L2:
      rl1->SequencePushSelf();
      rl2->SequencePushSelf();
      break;
  };
  RefBase::SequencePushSelf();
}


/*****
Put the construction of this line to a stream.
*****/
bool RefLine_L2L::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[03] ";
  os << "Fold ";
  switch (mWhoMoves) {
    case WHOMOVES_L1:
      rl1->PutName(os);
      os << " to ";
      rl2->PutName(os);   
      break;
    
    case WHOMOVES_L2:
      rl2->PutName(os);
      os << " to ";
      rl1->PutName(os);   
      break;
  };
  os << ", making ";
  PutName(os);
  if (ReferenceFinder::sClarifyVerbalAmbiguities) {
    os << " through ";
    
    // Now we need to specify which of the two bisectors this is, which we do
    // by specifying a point where the bisector hits the edge of the square.
    XYPt p;
    rl1->l.Intersects(rl2->l, p); // get the intersection of the two bisectors

    XYPt pa, pb;
    ReferenceFinder::sPaper.ClipLine(l, pa, pb);   // find where our fold line hits the paper.
    
    // Return the first point of intersection between the fold line and the edge of the
    // paper that _isn't_ the intersection of the two bisectors.
    os.precision(2);
    os.setf(ios_base::fixed, ios_base::floatfield);
	if (p == pa) {
		os << pb.Chop();
		return true;
	}
    os << pa.Chop();
  };
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_L2L::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow that brings two points from one line to
  // two points on the other line. We need to pick points that are within the
  // paper for both lines.
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
      
      XYLine& l1 = rl1->l;
      XYLine& l2 = rl2->l;
      XYPt p1a, p1b;
      ReferenceFinder::sPaper.ClipLine(l1, p1a, p1b);  // endpoints of l1
      XYPt p2a, p2b;
      ReferenceFinder::sPaper.ClipLine(l2, p2a, p2b);  // endpoints of l2
      p2a = l.Fold(p2a);                // flop l2 points onto l1
      p2b = l.Fold(p2b);
      XYPt du1 = l1.d * l1.u;       // a point on l1
      XYPt up1 = l1.u.Rotate90();     // a tangent to l1
      vector<double> tvals;       // holds parameterizations of the 4 points
      tvals.push_back((p1a - du1).Dot(up1));  // parameterize p1a along l1
      tvals.push_back((p1b - du1).Dot(up1));  // parameterize p1b along l1
      tvals.push_back((p2a - du1).Dot(up1));  // parameterize p2a along l1
      tvals.push_back((p2b - du1).Dot(up1));  // parameterize p2b along l1
      sort(tvals.begin(), tvals.end());   // sort them in order; we want the middle 2
      XYPt p1c = du1 + 0.5 * (tvals[1] + tvals[2]) * up1;
      XYPt p2c = l.Fold(p1c);
      switch(mWhoMoves) {
        case WHOMOVES_L1:
          sDgmr->DrawFoldAndUnfoldArrow(p1c, p2c);
          break;
        case WHOMOVES_L2:
          sDgmr->DrawFoldAndUnfoldArrow(p1c, p2c);
          break;
      }
  }
}


/*****
Go through existing lines and marks and create RefLine_L2Ls with rank equal to
arank up to a cumulative total of sMaxLines.
*****/
void RefLine_L2L::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1) / 2; irank++) {
    rank_t jrank = arank - irank - 1;
    bool sameRank = (irank == jrank);
    RefContainer<RefLine>::rank_iterator li = 
      ReferenceFinder::sBasisLines.maps[irank].begin();
    if (sameRank) li++;
    while (li != ReferenceFinder::sBasisLines.maps[irank].end()) {
      RefContainer<RefLine>::rank_iterator lj = 
        ReferenceFinder::sBasisLines.maps[jrank].begin();
      while (lj != (sameRank ? li : ReferenceFinder::sBasisLines.maps[jrank].end())) {
        if (ReferenceFinder::GetNumLines() >= ReferenceFinder::sMaxLines) return;
        RefLine_L2L rls1(li->second, lj->second, 0);
        ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rls1);
        if (ReferenceFinder::GetNumLines() >= ReferenceFinder::sMaxLines) return;
        RefLine_L2L rls2(li->second, lj->second, 1);
        ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rls2);
        lj++;
      };
      li++;
    }
  }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_L2L_C2P - Huzita-Hatori Axiom O4
Bring line l1 to itself so that the crease goes through point p1
**********/

/*****
Constructor.
*****/
RefLine_L2L_C2P::RefLine_L2L_C2P(RefLine* arl1, RefMark* arm1) : 
  RefLine(CalcLineRank(arl1, arm1)), rl1(arl1), rm1(arm1)
{     
  // Get references to line and mark
  XYPt& u1 = rl1->l.u;
  double& d1 = rl1->l.d;
  XYPt& p1 = rm1->p;
  
  // Construct the direction vector and distance scalar for the fold line.
  l.u = u1.Rotate90();
  l.d = p1.Dot(l.u);

  // The intersection of the fold line with line l1 must be enclosed in the paper.
  // That point is the projection of p1 onto line l1.
  XYPt p1p = p1 + (d1 - (p1.Dot(u1))) * u1;
  if (!ReferenceFinder::sPaper.Encloses(p1p)) return;
  
  // Don't need to check visibility, this kind is always visible.
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;
  
  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_L2L_C2P::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rm1);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_L2L_C2P::SequencePushSelf()
{
  rm1->SequencePushSelf();
  rl1->SequencePushSelf();
  RefBase::SequencePushSelf();
}


/*****
Put the construction of this line to a stream.
*****/
bool RefLine_L2L_C2P::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[04] ";
  os << "Fold ";
  rl1->PutName(os);
  os << " onto itself, making ";
  PutName(os);
  os << " through ";
  rm1->PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_L2L_C2P::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
      
      XYPt p1, p2;
      XYLine& l1 = rl1->l;
      ReferenceFinder::sPaper.ClipLine(l1, p1, p2);  // get endpts of the reference line
      XYPt pi = Intersection(l, l1);          // intersection w/ fold line
      XYPt u1p = l1.u.Rotate90();           // tangent to reference line
      double t1 = abs((p1 - pi).Dot(u1p));
      double t2 = abs((p2 - pi).Dot(u1p));
      double tmin = t1 < t2 ? t1 : t2;
      sDgmr->DrawFoldAndUnfoldArrow(pi + tmin * u1p, pi - tmin * u1p);
  }
}


/*****
Go through existing lines and marks and create RefLine_L2L_C2Ps with rank equal
to arank up to a cumulative total of sMaxLines.
*****/
void RefLine_L2L_C2P::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1); irank++) {
    rank_t jrank = arank - irank - 1;
    RefContainer<RefLine>::rank_iterator li = 
      ReferenceFinder::sBasisLines.maps[irank].begin();
    while (li != ReferenceFinder::sBasisLines.maps[irank].end()) {
      RefContainer<RefMark>::rank_iterator mj = 
        ReferenceFinder::sBasisMarks.maps[jrank].begin();
      while (mj != ReferenceFinder::sBasisMarks.maps[jrank].end()) {
        if (ReferenceFinder::GetNumLines() >= ReferenceFinder::sMaxLines) return;
        RefLine_L2L_C2P rls1(li->second, mj->second);
        ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rls1);
        mj++;
      };
      li++;
    }
  }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_P2L_C2P - Huzita-Hatori Axiom O5. 
Bring point p1 to line l1 so that the crease passes through point p2.
**********/

/*****
Constructor. iroot can be 0 or 1.
*****/
RefLine_P2L_C2P::RefLine_P2L_C2P(RefMark* arm1, RefLine* arl1, RefMark* arm2, short iroot) :
  RefLine(CalcLineRank(arm1, arl1, arm2)), rm1(arm1), rl1(arl1), rm2(arm2)
{
  // Get references to the points and lines.
  XYPt& p1 = rm1->p;
  XYLine& l1 = rl1->l;
  XYPt& u1 = l1.u;
  double& d1 = l1.d;
  XYPt& p2 = rm2->p;

  // If either point is already on the line, then this isn't interesting, i.e., it's
  // a trivial Haga construction.
  if (l1.Intersects(p1) || l1.Intersects(p2)) return;
  
  // Construct the line.
  double a = d1 - p2.Dot(u1);
  double b2 = (p2 - p1).Mag2() - a * a;
  
  if (b2 < 0) return;   // no solution for negative b2 (implies imaginary b)
  
  double b = sqrt(b2);
  if ((b < EPS) && (iroot == 1)) return;  // degenerate case, there's only one solution
  
  // Construct the image of p1 (p1p), which depends on which root we're after.
  XYPt u1p = u1.Rotate90();
  XYPt p1p = p2 + a * u1;
  if (iroot == 0) p1p += b * u1p;
  else p1p -= b * u1p;
  
  // Validate; the point of incidence must lie within the square.
  if (!ReferenceFinder::sPaper.Encloses(p1p)) return;
  
  // Construct member data.
  l.u = (p1p - p1).Normalize();
  l.d = p2.Dot(l.u);
  
  // Check visibility.
  bool p1edge = arm1->IsOnEdge();
  bool l1edge = arl1->IsOnEdge();
  
  if (ReferenceFinder::sVisibilityMatters) {
    if (p1edge) mWhoMoves = WHOMOVES_P1;
    else if (l1edge) mWhoMoves = WHOMOVES_L1;
    else return;
  }
  else {
    mWhoMoves = WHOMOVES_P1;
  };
  
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;
  
  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_P2L_C2P::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rm1 || rb == rm2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_P2L_C2P::SequencePushSelf()
{
  rm2->SequencePushSelf();
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rl1->SequencePushSelf();
      rm1->SequencePushSelf();
      break;
      
    case WHOMOVES_L1:
      rm1->SequencePushSelf();
      rl1->SequencePushSelf();

      break;
  };
  RefBase::SequencePushSelf();
}


/*****
Put the name of this line to a stream.
*****/
bool RefLine_P2L_C2P::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[05] ";
  os << "Bring ";
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rm1->PutName(os);
      os << " to ";
      rl1->PutName(os);
      break;
    
    case WHOMOVES_L1:
      rl1->PutName(os);
      os << " to ";
      rm1->PutName(os);
      break;
  };
  if (ReferenceFinder::sClarifyVerbalAmbiguities) {   
    os << " so the crease goes through ";
    rm2->PutName(os);
  };
  os << ", making ";
  PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_P2L_C2P::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow
  
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
      
    XYPt& p1 = rm1->p;
    XYPt p1f = l.Fold(p1);
    switch (mWhoMoves) {
      case WHOMOVES_P1:
        sDgmr->DrawFoldAndUnfoldArrow(p1, p1f);
        break;
      case WHOMOVES_L1:
        sDgmr->DrawFoldAndUnfoldArrow(p1f, p1);
        break;
    }
  }
}


/*****
Go through existing lines and marks and create RefLine_P2L_C2Ps with rank equal
arank up to a cumulative total of sMaxLines.
*****/
void RefLine_P2L_C2P::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1); irank++)
    for (rank_t jrank = 0; jrank <= (arank - 1 - irank); jrank++) {
      rank_t krank = arank - irank - jrank - 1;
      RefContainer<RefMark>::rank_iterator mi = 
        ReferenceFinder::sBasisMarks.maps[irank].begin();
      while (mi != ReferenceFinder::sBasisMarks.maps[irank].end()) {
        RefContainer<RefLine>::rank_iterator lj = 
          ReferenceFinder::sBasisLines.maps[jrank].begin();
        while (lj != ReferenceFinder::sBasisLines.maps[jrank].end()) {
          RefContainer<RefMark>::rank_iterator mk = 
            ReferenceFinder::sBasisMarks.maps[krank].begin();
          while (mk != ReferenceFinder::sBasisMarks.maps[krank].end()) {
            if ((irank != krank) || (mi != mk)) {   // only cmpr iterators if same container
              if (ReferenceFinder::GetNumLines() >= 
                ReferenceFinder::sMaxLines) return;
              RefLine_P2L_C2P rlh1(mi->second, lj->second, mk->second, 0);
              ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rlh1);
              if (ReferenceFinder::GetNumLines() >= 
                ReferenceFinder::sMaxLines) return;
              RefLine_P2L_C2P rlh2(mi->second, lj->second, mk->second, 1);
              ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rlh1);
            };
            mk++;
          };
          lj++;
        };
        mi++;
      }
    }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_P2L_P2L - Huzita-Hatori Axiom O6.
Bring point p1 to line l1 and point p2 to line l2.
**********/

/*****
* RefLine_P2L_P2L static member initialization
*****/
short RefLine_P2L_P2L::order = 0;
short RefLine_P2L_P2L::irootMax = 0;

double RefLine_P2L_P2L::q1 = 0;
double RefLine_P2L_P2L::q2 = 0;

double RefLine_P2L_P2L::S = 0;
double RefLine_P2L_P2L::Sr = 0;
double RefLine_P2L_P2L::Si = 0;
double RefLine_P2L_P2L::U = 0;


/*****
Take the cube root; works for both positive and negative numbers
*****/
double CubeRoot(double x)
{
  if (x >= 0) return pow(x, 1./3);
  else return -pow(-x, 1./3);
}


/*****
Constructor. Variable iroot can be 0, 1, or 2.
*****/
RefLine_P2L_P2L::RefLine_P2L_P2L(RefMark* arm1, RefLine* arl1, RefMark* arm2, 
  RefLine* arl2, short iroot) : 
  RefLine(CalcLineRank(arm1, arl1, arm2, arl2)), 
  rm1(arm1), 
  rl1(arl1), 
  rm2(arm2), 
  rl2(arl2)
{
  // Get references to the points and lines involved in the construction
  XYPt& p1 = rm1->p;
  XYLine& l1 = rl1->l;
  XYPt& u1 = l1.u;
  double& d1 = l1.d;
  XYPt& p2 = rm2->p;
  XYLine& l2 = rl2->l;
  XYPt& u2 = l2.u;
  double& d2 = l2.d;

  // This is by far the most complex alignment, and it involves the solution of
  // a cubic equation.
  XYPt u1p = u1.Rotate90(); // we'll need this later.
  
  // First, some trivial checks; we can't have p1 already on l1, or p2 already
  // on l2.
  if (l1.Intersects(p1)) return;
  if (l2.Intersects(p2)) return;
  
  // Also make sure we're using distinct points and lines.
  if ((p1 == p2) || (l1 == l2)) return;
  
  // Now construct the terms of the cubic equation. These are stored in static
  // member variables during the iroot==0 construction; if iroot==1 or 2, we
  // used the stored values.
  double rc = 0;  // this will hold the root of the cubic equation after this switch(iroot).
  switch (iroot) {
    case 0:
      { 
        // case iroot==0 computes a bunch of quantities that are used for all roots.
        // Some of these get stored in private static member variables.
        XYPt v1 = p1 + d1 * u1 - 2 * p2;
        XYPt v2 = d1 * u1 - p1;

        double c1 = p2.Dot(u2) - d2;
        double c2 = 2 * v2.Dot(u1p);
        double c3 = v2.Dot(v2);
        double c4 = (v1 + v2).Dot(u1p);
        double c5 = v1.Dot(v2);
        double c6 = u1p.Dot(u2);
        double c7 = v2.Dot(u2);
        
        // the equation is a * r^3 + b * r^2 + c * r + d == 0
        double a = c6;
        double b = c1 + c4 * c6 + c7;
        double c = c1 * c2 + c5 * c6 + c4 * c7;
        double d = c1 * c3 + c5 * c7;
    
        // compute the order of the equation
        if (abs(a) > EPS) order = 3;    // cubic equation
        else if (abs(b) > EPS) order = 2; // quadratic equation
        else if (abs(c) > EPS) order = 1; // linear equation
        else order = 0;           // ill-formed equation (no variables!)
        
        // what we do next depends on the order of the equation.
        switch(order) {
          case 0:       // ill-formed equation has 0 roots
            return;
          
          case 1:       // linear equation has 1 root
            {
              rc = -d / c;
            }
            break;
          
          case 2:       // quadratic equation has 0, 1 or 2 roots
            {       
              double disc = pow(c, 2) - 4 * b * d;
              q1 = -c / (2 * b);
              if (disc < 0) {
                irootMax = -1;        // no roots
                return;
              }
              else if (abs(disc) < EPS) {
                irootMax = 0;       // 1 degenerate root
                rc = q1;          // and here it is
              }
              else {
                irootMax = 1;       // 2 roots
                q2 = sqrt(disc) / (2 * b);
                rc = q1 + q2;       // and here's the first
              }
            }
            break;
        
          case 3:       // cubic equation, has 1, 2, or 3 roots
            {
              // Construct coefficients that give the roots from Cardano's formula.
              double a2 = b / a;
              double a1 = c / a;
              double a0 = d / a;
              
              double Q = (3 * a1 - pow(a2, 2)) / 9;
              double R = (9 * a2 * a1 - 27 * a0 - 2 * pow(a2, 3)) / 54;
              double D = pow(Q, 3) + pow(R, 2);
              U = -a2 / 3;
              
              // The number of roots depends on the value of D.
              
              if (D > 0) {
                irootMax = 0;       // one root
                double rD = sqrt(D);
                S = CubeRoot(R + rD);
                double T = CubeRoot(R - rD);
                rc = U + S + T;       // and here it is.
              }
              else if (abs(D) < EPS) {
                irootMax = 1;       // two roots
                S = pow(R, 1./3);
                rc = U + 2 * S;       // here's the first
              }
              else { // D < 0
                irootMax = 2;       // three roots
                double rD = sqrt(-D);
                double phi = atan2(rD, R) / 3;
                double rS = pow(pow(R, 2) - D, 1./6);
                Sr = rS * cos(phi);
                Si = rS * sin(phi);
                rc = U + 2 * Sr;      // here's the first
              }
            }
            break;  // end of case 3 of order
          }
      }   
      break;  // end of case 0 of iroot
    // for the other two roots, we'll rely on the fact that the coefficients
    // of the equation and first root have already been constructed.
    case 1: // of iroot, meaning we're looking for the second root
    
      if (irootMax < 1) return;
      switch(order) {
        case 2:
          rc = q1 - q2;   // second root of a quadratic
          break;
        
        case 3:         // second root of a cubic
          if (irootMax == 1)
            rc = U - S;
          else // irootMax == 2
            rc = U - Sr - sqrt(3.) * Si;
          break;
      };
      break;  // end of case 1 of iroot
    
    case 2: // of iroot, meaning we're looking for the third root
    
      if (irootMax < 2) return;
      switch(order) {
        case 3:         // third root of a cubic
          rc = U - Sr + sqrt(3.) * Si;
          break;
      };
      break;  // end of case 2 of iroot
      
  };  // end of switch(iroot).
  
  // If we're here, rc contains a root of the equation, which must still be validated.
  XYPt p1p = d1 * u1 + rc * u1p;          // image of p1 in fold line
  
  if (p1p == p1) return;              // we only consider p1 off of the fold line
  
  l.u = (p1p - p1).Normalize();         // normal to fold line
  l.d = l.u.Dot(MidPoint(p1p, p1));       // d-parameter of fold line
  XYPt p2p = p2 + 2 * (l.d - p2.Dot(l.u)) * l.u;  // image of p2 in fold line
  
  // Validate; the images of p1 and p2 must lie within the square.
  if (!ReferenceFinder::sPaper.Encloses(p1p) || 
    !ReferenceFinder::sPaper.Encloses(p2p)) return;
  
  // Validate visibility; we require that the alignment be visible even with
  // opaque paper. Meaning that the moving parts must be edge points or edge
  // lines.
  
  // Note whether p1 and p2 are on the same side of the fold line. If they are,
  // then either both points move or both lines move. If they're not, then one
  // of each moves.
  
  bool sameSide = ((p1.Dot(l.u) - l.d) * (p2.Dot(l.u) - l.d) >= 0);
  
  // Note which points and lines are on the edge of the paper
  bool p1edge = rm1->IsOnEdge();
  bool p2edge = rm2->IsOnEdge();
  bool l1edge = rl1->IsOnEdge();
  bool l2edge = rl2->IsOnEdge();
  
  // Now, check the visibility of this alignment and use it to specify which
  // parts move
  if (ReferenceFinder::sVisibilityMatters) {
    if (sameSide)
      if (p1edge && p2edge) mWhoMoves = WHOMOVES_P1P2;
      else if (l1edge && l2edge) mWhoMoves = WHOMOVES_L1L2;
      else return;
    else
      if (p1edge && l2edge) mWhoMoves = WHOMOVES_P1L2;
      else if (p2edge && l1edge) mWhoMoves = WHOMOVES_P2L1;
      else return;
  }
  else {
    if (sameSide) mWhoMoves = WHOMOVES_P1P2;
    else mWhoMoves = WHOMOVES_P1L2;
  };
  
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;
  
  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_P2L_P2L::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rm1 || rb == rl2 || rb == rm2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_P2L_P2L::SequencePushSelf()
{
  switch (mWhoMoves) {
    case WHOMOVES_P1P2:
      rl2->SequencePushSelf();
      rl1->SequencePushSelf();
      rm2->SequencePushSelf();
      rm1->SequencePushSelf();
      break;
      
    case WHOMOVES_L1L2:
      rm2->SequencePushSelf();
      rm1->SequencePushSelf();
      rl2->SequencePushSelf();
      rl1->SequencePushSelf();
      break;
    
    case WHOMOVES_P1L2:
      rm2->SequencePushSelf();
      rl1->SequencePushSelf();
      rl2->SequencePushSelf();
      rm1->SequencePushSelf();
      break;
    
    case WHOMOVES_P2L1:
      rl2->SequencePushSelf();
      rm1->SequencePushSelf();
      rl1->SequencePushSelf();
      rm2->SequencePushSelf();
      break;
  };
  RefBase::SequencePushSelf();
}


/*****
Put the name of this line to a stream.
*****/
bool RefLine_P2L_P2L::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[06] ";
  os.precision(2);
  os.setf(ios_base::fixed, ios_base::floatfield);
  os << "Bring ";
  switch (mWhoMoves) {
    case WHOMOVES_P1P2:
      rm1->PutName(os);
      os << " to ";
      rl1->PutName(os);
      if (ReferenceFinder::sClarifyVerbalAmbiguities)
        os << " at point " << l.Fold(rm1->p).Chop();
      os << " and ";
      rm2->PutName(os);
      os << " to ";
      rl2->PutName(os);
      break;
    
    case WHOMOVES_L1L2:
      rl1->PutName(os);
      if (ReferenceFinder::sClarifyVerbalAmbiguities)
        os << " so that point " << l.Fold(rm1->p).Chop();
      os << " touches ";
      rm1->PutName(os);
      os << " and ";
      rl2->PutName(os);
      os << " to ";
      rm2->PutName(os);
      break;
    
    case WHOMOVES_P1L2:
      rm1->PutName(os);
      os << " to ";
      rl1->PutName(os);
      if (ReferenceFinder::sClarifyVerbalAmbiguities)
        os << " at point " << l.Fold(rm1->p).Chop();
      os << " and ";
      rl2->PutName(os);
      os << " to ";
      rm2->PutName(os);
      break;
    
    case WHOMOVES_P2L1:
      rl1->PutName(os);
      os << " to ";
      rm1->PutName(os);
      os << " and ";
      rm2->PutName(os);
      os << " to ";
      rl2->PutName(os);
      if (ReferenceFinder::sClarifyVerbalAmbiguities)
        os << " at point " << l.Fold(rm2->p).Chop();
      break;
  };
  os << ", making ";
  PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_P2L_P2L::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
      
        XYPt& p1a = rm1->p;
        XYPt p1b = l.Fold(p1a);
        XYPt& p2a = rm2->p;
        XYPt p2b = l.Fold(p2a);
    switch (mWhoMoves) {
      case WHOMOVES_P1P2:
        sDgmr->DrawFoldAndUnfoldArrow(p1a, p1b);
        sDgmr->DrawFoldAndUnfoldArrow(p2a, p2b);
        break;
        
      case WHOMOVES_L1L2:
        sDgmr->DrawFoldAndUnfoldArrow(p1b, p1a);
        sDgmr->DrawFoldAndUnfoldArrow(p2b, p2a);
        break;
      
      case WHOMOVES_P1L2:
        sDgmr->DrawFoldAndUnfoldArrow(p1a, p1b);
        sDgmr->DrawFoldAndUnfoldArrow(p2b, p2a);
        break;
      
      case WHOMOVES_P2L1:
        sDgmr->DrawFoldAndUnfoldArrow(p1b, p1a);
        sDgmr->DrawFoldAndUnfoldArrow(p2a, p2b);
        break;
    }
  }
}


/*****
Go through existing lines and marks and create RefLine_P2L_P2Ls with rank equal
arank up to a cumulative total of sMaxLines.
*****/
void RefLine_P2L_P2L::MakeAll(rank_t arank)
{
  // psrank == sum of ranks of the two points
  // lsrank == sum of ranks of the two lines
  for (rank_t psrank = 0; psrank <= (arank - 1); psrank++)
    for (rank_t lsrank = 0; lsrank <= (arank - 1) - psrank; lsrank++)

      // point order doesn't matter, so rank(pt[i]) will always be <= rank(pt[j])
      for (rank_t irank = 0; irank <= psrank / 2; irank++) {
        rank_t jrank = psrank - irank;
        bool psameRank = (irank == jrank);
        
        // line order does matter, so both lines vary over all ranks
        for (rank_t krank = 0; krank <= lsrank; krank++)
          for (rank_t lrank = 0; lrank <= lsrank - krank; lrank++) {
    
            // iterate over all combinations of points & lines with given rank
            RefContainer<RefMark>::rank_iterator mi = 
              ReferenceFinder::sBasisMarks.maps[irank].begin();
            if (psameRank) mi++;
            while (mi != ReferenceFinder::sBasisMarks.maps[irank].end()) {
              RefContainer<RefMark>::rank_iterator mj = 
                ReferenceFinder::sBasisMarks.maps[jrank].begin();
              while (mj != (psameRank ? mi : 
                ReferenceFinder::sBasisMarks.maps[jrank].end())) {
                RefContainer<RefLine>::rank_iterator lk = 
                  ReferenceFinder::sBasisLines.maps[krank].begin();
                while (lk != ReferenceFinder::sBasisLines.maps[krank].end()) {
                  RefContainer<RefLine>::rank_iterator ll = 
                    ReferenceFinder::sBasisLines.maps[lrank].begin();
                  while (ll != ReferenceFinder::sBasisLines.maps[lrank].end()) {
                    if ((krank != lrank) || (lk != ll)) {   // cmpr iterators only if same container
                      if (ReferenceFinder::GetNumLines() >= 
                        ReferenceFinder::sMaxLines) return;
                      RefLine_P2L_P2L rlp0(mi->second, lk->second, 
                        mj->second, ll->second, 0);
                      ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(
                        rlp0);
                      if (ReferenceFinder::GetNumLines() >= 
                        ReferenceFinder::sMaxLines) return;
                      RefLine_P2L_P2L rlp1(mi->second, lk->second, 
                        mj->second, ll->second, 1);
                      ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(
                        rlp1);
                      if (ReferenceFinder::GetNumLines() >= 
                        ReferenceFinder::sMaxLines) return;
                      RefLine_P2L_P2L rlp2(mi->second, lk->second, 
                        mj->second, ll->second, 2);
                      ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(
                        rlp2);
                    };
                    ll++;
                  };
                  lk++;
                };
                mj++;
              };
              mi++;
            }
          }
        }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RefLine_L2L_P2L - Huzita-Hatori Axiom O7 (Hatori's Axiom).
Bring line l1 onto itself so that point p1 falls on line l2.
**********/

/*****
Constructor. iroot can be 0 or 1.
*****/
RefLine_L2L_P2L::RefLine_L2L_P2L(RefLine* arl1, RefMark* arm1, RefLine* arl2) :
  RefLine(CalcLineRank(arl1, arm1, arl2)), rl1(arl1), rm1(arm1), rl2(arl2)
{
  // Get references
  XYLine& l1 = rl1->l;
  XYPt& u1 = l1.u;
  double& d1 = l1.d;
  XYPt& p1 = rm1->p;
  XYLine& l2 = rl2->l;
  XYPt& u2 = l2.u;
  
  // Construct direction vector and distance scalar
  l.u = u2.Rotate90();
  
  double uf1 = l.u.Dot(u1);
  if (abs(uf1) < EPS) return; // parallel lines, no solution
  
  l.d = (d1 + 2 * p1.Dot(l.u) * uf1 - p1.Dot(u1)) / (2 * uf1);
  
  // Make sure point of intersection of fold with l2 lies within the paper.
  XYPt pt = Intersection(l, l2);
  if (!ReferenceFinder::sPaper.Encloses(pt)) return;
  
  // Make sure point of incidence of p1 on l1 lies within the paper.
  XYPt p1p = l.Fold(p1);
  if (!ReferenceFinder::sPaper.Encloses(p1p)) return;
  
  // Make sure p1 isn't already on l1 (in which case the alignment is ill-defined).
  if (l1.Intersects(p1)) return;
  
  // Check visibility.
  bool p1edge = arm1->IsOnEdge();
  bool l1edge = arl1->IsOnEdge();
  
  if (ReferenceFinder::sVisibilityMatters) {
    XYPt lp1, lp2;
    ReferenceFinder::sPaper.ClipLine(l, lp1, lp2);
    double t1 = (lp1 - pt).Dot(l.u);
    double t2 = (lp2 - pt).Dot(l.u);
    double tp = (p1 - pt).Dot(l.u);
    if ((t1 * tp) < 0) {
      double ti = t2;
      t2 = t1;
      t1 = ti;
    };  // now t1 is the parameter for the endpoint on the p1 side of l2.
    if (p1edge && (abs(t1) <= abs(t2))) mWhoMoves = WHOMOVES_P1;
    else if (l1edge && (abs(t1) >= abs(t2))) mWhoMoves = WHOMOVES_L1;
    else return;
  }
  else {
    mWhoMoves = WHOMOVES_P1;
  };
    
  // If this line creates a skinny flap, we won't use it.
  if (ReferenceFinder::sPaper.MakesSkinnyFlap(l)) return;  
  
  // Set the key.
  FinishConstructor();
}


/*****
Return true if this line uses rb for immediate reference.
*****/
bool RefLine_L2L_P2L::UsesImmediate(RefBase* rb) const
{
  return (rb == rl1 || rb == rm1 || rb == rl2);
}


/*****
Build the folding sequence that constructs this object.
*****/
void RefLine_L2L_P2L::SequencePushSelf()
{
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rl1->SequencePushSelf();
      rm1->SequencePushSelf();
      break;

    case WHOMOVES_L1:
      rm1->SequencePushSelf();
      rl1->SequencePushSelf();
      break;
  };
  rl2->SequencePushSelf(); 
  RefBase::SequencePushSelf();
}
      

/*****
Put the name of this line to a stream.
*****/
bool RefLine_L2L_P2L::PutHowto(ostream& os) const
{
  if (ReferenceFinder::sAxiomsInVerbalDirections) os << "[07] ";
  os << "Bring ";
  rl2->PutName(os);
  os << " onto itself so that ";
  switch (mWhoMoves) {
    case WHOMOVES_P1:
      rm1->PutName(os);
      os << " touches ";
      rl1->PutName(os);
      break;
    
    case WHOMOVES_L1:
      rl1->PutName(os);
      os << " touches ";
      rm1->PutName(os);
      break;
  };
  os << ", making ";
  PutName(os);
  return true;
}


/*****
Draw this line, adding arrows if appropriate
*****/
void RefLine_L2L_P2L::DrawSelf(RefStyle rstyle, short ipass) const
{
  // Call inherited method to draw the lines
  RefLine::DrawSelf(rstyle, ipass);
  
  // If we're moving, we need an arrow
  if ((ipass == PASS_ARROWS) && (rstyle == REFSTYLE_ACTION)) {
      
    // Draw line-to-itself arrow
    XYPt p1, p2;
    XYLine& l2 = rl2->l;
    ReferenceFinder::sPaper.ClipLine(l2, p1, p2);  // get endpts of the reference line
    XYPt pi = Intersection(l, l2);          // intersection w/ fold line
    XYPt u1p = l2.u.Rotate90();           // tangent to reference line
    double t1 = abs((p1 - pi).Dot(u1p));
    double t2 = abs((p2 - pi).Dot(u1p));
    double tmin = t1 < t2 ? t1 : t2;
    sDgmr->DrawFoldAndUnfoldArrow(pi + tmin * u1p, pi - tmin * u1p);
    
    // Draw point-to-line arrow
    XYPt& p3 = rm1->p;
    XYPt p3p = l.Fold(p3);
    switch(mWhoMoves) {
      case WHOMOVES_P1:
        sDgmr->DrawFoldAndUnfoldArrow(p3, p3p);
        break;
      case WHOMOVES_L1:
        sDgmr->DrawFoldAndUnfoldArrow(p3p, p3);
        break;
    }
  }
}


/*****
Go through existing lines and marks and create RefLine_L2L_P2Ls with rank equal
arank up to a cumulative total of sMaxLines.
*****/
void RefLine_L2L_P2L::MakeAll(rank_t arank)
{
  for (rank_t irank = 0; irank <= (arank - 1); irank++)
    for (rank_t jrank = 0; jrank <= (arank - 1 - irank); jrank++) {
      rank_t krank = arank - irank - jrank - 1;
      RefContainer<RefLine>::rank_iterator li = 
        ReferenceFinder::sBasisLines.maps[irank].begin();
      while (li != ReferenceFinder::sBasisLines.maps[irank].end()) {
        RefContainer<RefMark>::rank_iterator mj = 
          ReferenceFinder::sBasisMarks.maps[jrank].begin();
        while (mj != ReferenceFinder::sBasisMarks.maps[jrank].end()) {
          RefContainer<RefLine>::rank_iterator lk = 
            ReferenceFinder::sBasisLines.maps[krank].begin();
          while (lk != ReferenceFinder::sBasisLines.maps[krank].end()) {
            if ((irank != krank) || (li != lk)) {
              if (ReferenceFinder::GetNumLines() >= 
                ReferenceFinder::sMaxLines) return;
              RefLine_L2L_P2L rlh1(li->second, mj->second, lk->second);
              ReferenceFinder::sBasisLines.AddCopyIfValidAndUnique(rlh1);
            };
            lk++;
          };
          mj++;
        };
        li++;
      }
    }
}


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 3: containers for collections of marks and lines These containers are
templated on the object type; we use the same type for both RefMarks and
RefLines.
******************************************************************************/

/**********
class RefContainer - a container for storing RefMark* and RefLine* that
organizes them by rank and within a rank, uses a map<R*> to store them. The
mKey member variable is used as the key in the map; only one object is stored
per key. class R = RefMark or RefLine
**********/

/*****
Constructor. Initialize arrays.
*****/
template <class R>
RefContainer<R>::RefContainer() : rcsz(0), rcbz(0)
{
  // expand our map array to hold all the ranks that we will create
  maps.resize(1 + ReferenceFinder::sMaxRank);
}


/*****
Check the validity and uniqueness of object ars of type Rs (descended from type
R), and if it passes, add it to the container. Used by all the MakeAll()
functions. Note that we use the default (compiler-generated) copy constructor,
which is OK since our objects only contain POD, pointers that we WANT
blind-copied, and/or strings (which know how to copy themselves).
*****/
template <class R>
template <class Rs>
void RefContainer<R>::AddCopyIfValidAndUnique(const Rs& ars)
{
  // The ref is valid (fully constructed) if its key is something other than 0.
  // It's unique if the container doesn't already have one with the same key in
  // one of the rank maps.
  if (ars.mKey != 0 && !Contains(&ars)) Add(new Rs(ars));
  ReferenceFinder::CheckDatabaseStatus();  // report progress if appropriate

}


/*****
Rebuild all arrays and related counters.
*****/
template <class R>
void RefContainer<R>::Rebuild()
{
  rcsz = 0;
  rcbz = 0;
  this->resize(0);
  maps.resize(0);
  maps.resize(1 + ReferenceFinder::sMaxRank);
}


/*****
Return true if the container (or the buffer) contain an equivalent object.
*****/
template <class R>
bool RefContainer<R>::Contains(const R* ar) const
{
  // go through each rank and look for an object with the same key. If we find one,
  // return true.
  for (size_t ir = 0; ir < maps.size(); ir++) {
    if (maps[ir].count(ar->mKey)) return true;
  }
  
  // Also check the buffer.
  if (buffer.count(ar->mKey)) return true;
  
  // Still here? then we didn't find it.
  return false;
}


/*****
Add an element to the array; to be called only if an equivalent element isn't
already present anywhere. To avoid corrupting iterators, we add the new element
to the buffer; it will get added to the main container on the next call to
FlushBuffer().
*****/
template<class R>
void RefContainer<R>::Add(R* ar)
{
  // Add it to the buffer and increment the buffer size.
  buffer.insert(typename map_t::value_type(ar->mKey, ar));
  rcbz++;
}


/*****
Put the contents of the buffer into the main container.
*****/
template <class R>
void RefContainer<R>::FlushBuffer()
{
  // Make room for the buffer in the sortable list.
  
  reserve(this->size() + rcbz);
  
  // Go through the buffer and add each element to the appropriate rank in the main container.
  
  rank_iterator bi = buffer.begin();
  while (bi != buffer.end()) {
    R*& rr = bi->second;        // get pointer to each new element
    maps[rr->mRank].insert(*bi);    // add to the map of the appropriate rank
    push_back(rr);            // also add to our sortable list
    rcsz++;               // increment our size counter
    bi++;               // increment the buffer iterator
  };
  buffer.clear();             // clear the buffer
  rcbz = 0;
}


/*****
Clear the map arrays. Called when they're no longer needed.
*****/
template <class R>
void RefContainer<R>::ClearMaps()
{
  for (size_t ir = 0; ir < maps.size(); ir++) maps[ir].clear();
}


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 4: Routines for drawing diagrams
******************************************************************************/

/**********
class RefDgmr - object that draws folding diagrams of references. Subclasses
specialize to particular drawing environments (print vs screen, multiple GUIs,
platform-specific drawing models, etc.
**********/

/*  Notes on RefDgmr and its descendants.
Subclasses will typically maintain an internal graphics state that describes
the location of the diagram on the canvas and will implement the handful of
virtual drawing methods that render the diagram on the canvas. The base class
does nothing. All points are given in the paper coordinate system. The subclass
implementation of drawing should determine the layout of the diagrams and
offset and scale appropriately to convert to canvas coordinates. See
PSStreamDgmr for an example.
*/  

/*****
Draw a point in the given style.
*****/
void RefDgmr::DrawPt(const XYPt& /* aPt */, PointStyle /* pstyle */)
{
}


/*****
Draw a line in the given style.
*****/
void RefDgmr::DrawLine(const XYPt& /* fromPt */, const XYPt& /* toPt */, 
  LineStyle /* lstyle */)
{
}


/*****
Draw an arc in the given style. fromAngle and toAngle are given in radians.
*****/
void RefDgmr::DrawArc(const XYPt& /* ctr */, double /* rad */, 
  double /* fromAngle */, double /* toAngle */, bool /* ccw */, 
  LineStyle /* lstyle */)
{
}


/*****
Draw a polygon in the given style.
*****/
void RefDgmr::DrawPoly(const vector<XYPt>& /* poly */, PolyStyle /* pstyle */)
{
}


/*****
Draw a label at a given point
*****/
void RefDgmr::DrawLabel(const XYPt& /* aPt */, const string& /* aString */, 
  LabelStyle /* lstyle */)
{
}


/*
Class RefDgmr provides a set of routines for drawing arrows that are built on
top of the primitive routines above. These can be overridden if you want to
implement a different style of arrow.
*/

/*****
Draw a valley-fold arrowhead with tip at location loc, direction dir, and size
len.
*****/
void RefDgmr::DrawValleyArrowhead(const XYPt& loc, const XYPt& dir, double len)
{
  DrawLine(loc, loc - len * dir.RotateCCW(.523), LINESTYLE_ARROW);
  DrawLine(loc, loc - len * dir.RotateCCW(-.523), LINESTYLE_ARROW);
}


/*****
Draw a mountain arrowhead with tip at location loc, direction dir, and size len.
*****/
void RefDgmr::DrawMountainArrowhead(const XYPt& loc, const XYPt& dir, double len)
{
  XYPt ldir = len * dir;
  vector<XYPt> poly;
  poly.push_back(loc);
  poly.push_back(loc - ldir.RotateCCW(.523));
  poly.push_back(loc - .8 * ldir);
  DrawPoly(poly, POLYSTYLE_ARROW);
}


/*****
Draw an unfold arrowhead with tip at location loc, direction dir, and size len.
*****/
void RefDgmr::DrawUnfoldArrowhead(const XYPt& loc, const XYPt& dir, double len)
{
  XYPt ldir = len * dir;
  vector<XYPt> poly;
  poly.push_back(loc);
  poly.push_back(loc - ldir.RotateCCW(.523));
  poly.push_back(loc - .8 * ldir);
  poly.push_back(loc - ldir.RotateCCW(-.523));
  DrawPoly(poly, POLYSTYLE_ARROW);
}


/*****
Calculate all the parameters necessary to draw any type of arrow (valley,
mountain, unfold, fold-and-unfold).
*****/
void RefDgmr::CalcArrow(const XYPt& fromPt, const XYPt& toPt,
  XYPt& ctr, double& rad, double& fromAngle, double& toAngle, bool& ccw,
  double& ahSize, XYPt& fromDir, XYPt& toDir)
{
  const double RADIANS = 57.29577951;
  const double TWO_PI = 6.283185308;
  const double PI = 3.1415926535;
  
  const double ha = 30 / RADIANS;     // half-angle of arc of arrow, in degrees
  const double tana = tan(ha);      // tan of this angle
  
  XYPt mp = MidPoint(fromPt, toPt);   // midpoint of arrow line
  XYPt mu = (toPt - fromPt);        // vector in direction of arrow line
  XYPt mup = 0.5 * mu.Rotate90() / tana;  // vector from midpt to center of curvature
  
  // Compute the center of rotation. There are two possible choices.
  // We'll want the bulge of the arc to always be toward the inside of the square,
  // i.e., closer to the middle of the square, so we pick the value of the center
  // that's farther away.
  XYPt sqmp = MidPoint(ReferenceFinder::sPaper.mBotLeft, 
    ReferenceFinder::sPaper.mTopRight);
  XYPt ctr1 = mp + mup;
  XYPt ctr2 = mp - mup;
  ctr = (ctr1 - sqmp).Mag() > (ctr2 - sqmp).Mag() ? ctr1 : ctr2;

  // radius of the arc.
  rad = (toPt - ctr).Mag();
  
  // Now compute the angles of the lines to the two points.
  XYPt fp = fromPt - ctr;
  fromAngle = atan2(fp.y, fp.x);
  XYPt tp = toPt - ctr;
  toAngle = atan2(tp.y, tp.x);
  
  // Check direction of rotation.
  double ra = toAngle - fromAngle;  // rotation angle
  while (ra < 0) ra += TWO_PI;    // get it into the right range
  while (ra > TWO_PI) ra -= TWO_PI; 
  ccw = (ra < PI);          // true == arc goes in ccw direction
  
  // Compute the size of the arrowheads
  ahSize = ReferenceFinder::sPaper.mWidth;
  if (ahSize > ReferenceFinder::sPaper.mHeight) {
    ahSize = ReferenceFinder::sPaper.mHeight;
  }
  ahSize *= 0.15;
  double ah1 = 0.4 * (toPt - fromPt).Mag();
  if (ahSize > ah1) ahSize = ah1;
  
  // Compute the direction vectors for the arrowheads
  mu.NormalizeSelf();
  toDir = ccw ? mu.RotateCCW(ha) : mu.RotateCCW(-ha);
  mu *= -1;
  fromDir = ccw ? mu.RotateCCW(-ha) : mu.RotateCCW(ha);
}


/*****
Draw a valley-fold arrow. fromPt is the moving point, toPt is the destination.
*****/
void RefDgmr::DrawValleyArrow(const XYPt& fromPt, const XYPt& toPt)
{
  XYPt ctr;
  double rad;
  double fromAngle;
  double toAngle;
  bool ccw;
  double ahSize;
  XYPt fromDir;
  XYPt toDir;
  CalcArrow(fromPt, toPt, ctr, rad, fromAngle, toAngle, ccw, ahSize, fromDir, 
    toDir);
  DrawArc(ctr, rad, fromAngle, toAngle, ccw, LINESTYLE_ARROW);
  DrawValleyArrowhead(toPt, toDir, ahSize);
}


/*****
Draw a mountain-fold arrow. fromPt is the moving point, toPt is the
destination.
*****/
void RefDgmr::DrawMountainArrow(const XYPt& fromPt, const XYPt& toPt)
{
  XYPt ctr;
  double rad;
  double fromAngle;
  double toAngle;
  bool ccw;
  double ahSize;
  XYPt fromDir;
  XYPt toDir;
  CalcArrow(fromPt, toPt, ctr, rad, fromAngle, toAngle, ccw, ahSize, fromDir, 
    toDir);
  DrawArc(ctr, rad, fromAngle, toAngle, ccw, LINESTYLE_ARROW);
  DrawMountainArrowhead(toPt, toDir, ahSize);
}


/*****
Draw an unfold arrow. fromPt is the moving point, toPt is the destination.
*****/
void RefDgmr::DrawUnfoldArrow(const XYPt& fromPt, const XYPt& toPt)
{
  XYPt ctr;
  double rad;
  double fromAngle;
  double toAngle;
  bool ccw;
  double ahSize;
  XYPt fromDir;
  XYPt toDir;
  CalcArrow(fromPt, toPt, ctr, rad, fromAngle, toAngle, ccw, ahSize, fromDir, 
    toDir);
  DrawArc(ctr, rad, fromAngle, toAngle, ccw, LINESTYLE_ARROW);
  DrawUnfoldArrowhead(toPt, toDir, ahSize);
}


/*****
Draw a fold-and-unfold arrow. fromPt is the moving point, toPt is the
destination.
*****/
void RefDgmr::DrawFoldAndUnfoldArrow(const XYPt& fromPt, const XYPt& toPt)
{
  XYPt ctr;
  double rad;
  double fromAngle;
  double toAngle;
  bool ccw;
  double ahSize;
  XYPt fromDir;
  XYPt toDir;
  CalcArrow(fromPt, toPt, ctr, rad, fromAngle, toAngle, ccw, ahSize, fromDir, 
    toDir);
  DrawArc(ctr, rad, fromAngle, toAngle, ccw, LINESTYLE_ARROW);
  DrawValleyArrowhead(toPt, toDir, ahSize);
  DrawUnfoldArrowhead(fromPt, fromDir, ahSize);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class VerbalStreamDgmr - a minimal subclass of RefDgmr that puts verbal-only
descriptions to a stream.
**********/

/* Notes on class VerbalStreamDgmr.
This class doesn't do any drawing; instead, it puts a verbal description of the
fold steps to a stream. It is the outputter for the console-based version of
ReferenceFinder.
*/

/*****
Constructor
******/
VerbalStreamDgmr::VerbalStreamDgmr(ostream& aStream) : 
  RefDgmr(),
  mStream(&aStream)
{
  ReferenceFinder::sClarifyVerbalAmbiguities = true;
  ReferenceFinder::sAxiomsInVerbalDirections = true;
}


/*****
Write a list of references to the stream, along with their error and how-to
description. This template function is used in the mark- and line-specific
routines that follow.
*****/
template <class R>
void VerbalStreamDgmr::PutRefList(const typename R::bare_t& ar, vector<R*>& vr)
{
  (*mStream) << endl;
  for (size_t i = 0; i < vr.size(); i++) {
    vr[i]->PutDistanceAndRank(*mStream, ar);
    (*mStream) << endl;
    vr[i]->PutHowtoSequence(*mStream);
  };
  (*mStream) << endl;
}


/*****
Write a list of lines to the stream, including error and how-to description.
*****/
void VerbalStreamDgmr::PutLineList(const XYLine& ll, vector<RefLine*>& vl)
{
  PutRefList(ll, vl);
}


/*****
Write a list of marks to the stream, including error and how-to description.
*****/
void VerbalStreamDgmr::PutMarkList(const XYPt& pp, vector<RefMark*>& vm)
{
  PutRefList(pp, vm);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class PSStreamDgmr - a specialization of RefDgmr that writes a PostScript
stream of diagrams.
**********/

/* Notes on class PSStreamDgmr.
This class is used in the command-line version of ReferenceFinder to create
a Postscript graphics file of folding diagrams. It's also a good model for how
to implement a true graphic outputter.
*/

/*****
PSStreamDgmr static member initialization
*****/
double PSStreamDgmr::sPSUnit = 64;    // 72 pts = 1 inch, 1 unit = 64 pts, fits 7 dgms
const XYRect PSStreamDgmr::sPSPageSize(40, 40, 572, 752);  // printable area on the page


/*****
Constructor
*****/
PSStreamDgmr::PSStreamDgmr(ostream& os) :
   mStream(&os),
   mPSPageCount(0)
{
}


/*****
Stream output for a PostScript point
*****/
ostream& operator<<(ostream& os, const PSStreamDgmr::PSPt& pp)
{
  return os << pp.px << " " << pp.py;
}


/*****
Set the current graphics state to the given PointStyle.
*****/
void PSStreamDgmr::SetPointStyle(PointStyle pstyle)
{
  switch (pstyle) {
    case POINTSTYLE_NORMAL:
      (*mStream) << "1 setlinewidth 0 setgray " << endl;
      break;
    case POINTSTYLE_HILITE:
      (*mStream) << "3 setlinewidth .5 .25 .25 setrgbcolor " << endl;
      break;
    case POINTSTYLE_ACTION:
      (*mStream) << "3 setlinewidth .5 0 0 setrgbcolor " << endl;
      break;
  }
}


/*****
Set the current graphics state to the given LineStyle
*****/
void PSStreamDgmr::SetLineStyle(LineStyle lstyle)
{
  switch (lstyle) {
    case LINESTYLE_CREASE:
      (*mStream) << "[] 0 setdash .20 setlinewidth 0 setgray " << endl;
      break;
    case LINESTYLE_EDGE:
      (*mStream) << "[] 0 setdash .5 setlinewidth 0 setgray " << endl;
      break;
    case LINESTYLE_HILITE:
      (*mStream) << "[] 0 setdash 1 setlinewidth 1 .5 .5 setrgbcolor " << endl;
      break;
    case LINESTYLE_VALLEY:
      (*mStream) << "[4 3] 0 setdash .5 setlinewidth .5 .5 0 setrgbcolor " << endl;
      break;
    case LINESTYLE_MOUNTAIN:
      (*mStream) << "[3 3 0 3 0 3] 0 setdash .5 setlinewidth 0 0 0 setrgbcolor " << 
        endl;
      break;
    case LINESTYLE_ARROW:
      (*mStream) << "[] 0 setdash .4 setlinewidth 0 .5 0 setrgbcolor " << endl;
      break;
  }
}


/*****
Set the current graphics state to the given PolyStyle
*****/
void PSStreamDgmr::SetPolyStyle(PolyStyle pstyle)
{
  switch (pstyle) {
    case POLYSTYLE_WHITE:
      (*mStream) << ".95 .95 1 setrgbcolor " << endl;
      break;
    case POLYSTYLE_COLORED:
      (*mStream) << "0 0 .5 setrgbcolor " << endl;
      break;
    case POLYSTYLE_ARROW:
      (*mStream) << ".95 1 .95 setrgbcolor " << endl;
      break;
  }
}


/*****
Set the current graphics state to the given LabelStyle
*****/
void PSStreamDgmr::SetLabelStyle(LabelStyle lstyle)
{
  switch (lstyle) {
    case LABELSTYLE_NORMAL:
      (*mStream) << "0 setgray " << endl;
      break;
    case LABELSTYLE_HILITE:
      (*mStream) << ".5 .25 .25 setrgbcolor " << endl;
      break;
    case LABELSTYLE_ACTION:
      (*mStream) << ".5 0 0 setrgbcolor " << endl;
      break;
  }
}


/*****
Coordinate conversion
*****/
PSStreamDgmr::PSPt PSStreamDgmr::ToPS(const XYPt& aPt)
{
  return PSPt(mPSOrigin.x + sPSUnit * aPt.x, mPSOrigin.y + sPSUnit * aPt.y);
}


/*****
Draw a PostScript point in the indicated style.
*****/
void PSStreamDgmr::DrawPt(const XYPt& aPt, PointStyle pstyle)
{
  SetPointStyle(pstyle);
  (*mStream) << "newpath " << ToPS(aPt) << " moveto 0 0 rlineto stroke" << endl;
}


/*****
Draw a PostScript line in the indicated style.
*****/
void PSStreamDgmr::DrawLine(const XYPt& fromPt, const XYPt& toPt, 
  LineStyle lstyle)
{
  SetLineStyle(lstyle);
  (*mStream) << "newpath " << ToPS(fromPt) << " moveto " << ToPS(toPt) << 
    " lineto stroke" << endl;
}


/*****
Draw a PostScript arc in the indicated style.
*****/
void PSStreamDgmr::DrawArc(const XYPt& ctr, double rad, double fromAngle,
  double toAngle, bool ccw, LineStyle lstyle)
{
  SetLineStyle(lstyle);
  const double RADIANS = 57.29577951;
  if (ccw)
    (*mStream) << "newpath " << ToPS(ctr) << " " << rad * sPSUnit << " " << 
    fromAngle * RADIANS << " " << toAngle * RADIANS  << " arc stroke" << endl;
  else
    (*mStream) << "newpath " << ToPS(ctr) << " " << rad * sPSUnit << " " << 
    fromAngle * RADIANS  << " " << toAngle * RADIANS  << " arcn stroke" << endl;
}


/*****
Fill and stroke the given poly in the indicated style.
*****/
void PSStreamDgmr::DrawPoly(const vector<XYPt>& poly, PolyStyle pstyle)
{
  (*mStream) << "newpath " << ToPS(poly[poly.size()-1]) << " moveto " << endl;
  for (size_t i = 0; i < poly.size(); i++)
    (*mStream) << ToPS(poly[i]) << " lineto" << endl;
  (*mStream) << "gsave " << endl;

  // Fill the poly
  SetPolyStyle(pstyle);
  (*mStream) << "fill grestore " << endl;
  
  // Stroke the poly
  switch (pstyle) {
    case POLYSTYLE_WHITE:
    case POLYSTYLE_COLORED:
      SetLineStyle(LINESTYLE_EDGE);
      break;
    case POLYSTYLE_ARROW:
      SetLineStyle(LINESTYLE_ARROW);
      break;
  };
  (*mStream) << "stroke " << endl;
}


/*****
Draw a text label at the point aPt in the indicated style
*****/
void PSStreamDgmr::DrawLabel(const XYPt& aPt, const string& aString, 
  LabelStyle lstyle)
{
  SetLabelStyle(lstyle);
  (*mStream) << ToPS(aPt) << " moveto (" << aString << ") show " << endl;
}


/*****
Decrement the mPSOrigin by a distance d. If we drop below the bottom margin,
start a new page.
*****/

void PSStreamDgmr::DecrementOrigin(double d)
{
  mPSOrigin.y -= d;
  if (mPSOrigin.y >= sPSPageSize.bl.y) return;
  (*mStream) << "showpage" << endl;
  (*mStream) << "%%Page: " << ++mPSPageCount;
  (*mStream) << " " << mPSPageCount << endl;
  mPSOrigin.y = sPSPageSize.tr.y - d;
}

  
/*****
Draw a set of marks or lines to a PostScript stream, showing distance and rank
for each sequence.
*****/
template <class R>
void PSStreamDgmr::PutRefList(const typename R::bare_t& ar, vector<R*>& vr)
{
  ReferenceFinder::sClarifyVerbalAmbiguities = false;
  ReferenceFinder::sAxiomsInVerbalDirections = false;

  // Put some comments so our readers are happy 
  (*mStream) << "%!PS-Adobe-1.0" << endl;
  (*mStream) << "%%Pages: (atend)" << endl;
  (*mStream) << "%%EndComments" << endl;
  (*mStream) << "%%Page: 1 1" << endl;
  
  // Set the page number. DecrementOrigin will update it as needed
  mPSPageCount = 1;
  
  // Put some initial setup information 
  (*mStream) << "1 setlinecap" << endl;
  (*mStream) << "1 setlinejoin" << endl;
  
  // Setup and draw a header. 
  mPSOrigin.x = sPSPageSize.bl.x;
  mPSOrigin.y = sPSPageSize.tr.y;
  (*mStream) << "/Times-Roman findfont 12 scalefont setfont" << endl;
  (*mStream) << "0 setgray" << endl;
  DecrementOrigin(12);
  DrawLabel(XYPt(0), "ReferenceFinder 4.0 by Robert J. Lang", LABELSTYLE_NORMAL);
  
  // Note the point we're searching for.
  (*mStream) << "/Times-Roman findfont 9 scalefont setfont" << endl;
  DecrementOrigin(12);
  stringstream targstr;
  targstr << "Paper: \\(" << ReferenceFinder::sPaper.mWidthAsText.c_str()
	  << " x " << ReferenceFinder::sPaper.mHeightAsText.c_str()
	  << "\\), Target: " << ar;
  DrawLabel(XYPt(0), targstr.str(), LABELSTYLE_NORMAL);
  
  // Go through our list and draw all the diagrams in a single row. 
  for (size_t irow = 0; irow < vr.size(); irow++) {
    DecrementOrigin(1.2 * sPSUnit * ReferenceFinder::sPaper.mHeight);
    vr[irow]->BuildDiagrams();
    mPSOrigin.x = sPSPageSize.bl.x;
    for (size_t icol = 0; icol < RefBase::sDgms.size(); icol++) {
      RefBase::DrawDiagram(*this, RefBase::sDgms[icol]);
      mPSOrigin.x += 1.2 * ReferenceFinder::sPaper.mWidth * sPSUnit;
    };
    
    // Also put the text description below the diagrams   
    mPSOrigin.x = sPSPageSize.bl.x;
    DecrementOrigin(11);
    ostringstream sd;
    vr[irow]->PutDistanceAndRank(sd, ar);
    DrawLabel(XYPt(0), sd.str(), LABELSTYLE_NORMAL);
    for (size_t i = 0; i < RefBase::sSequence.size(); i++) {
      mPSOrigin.x = sPSPageSize.bl.x;
      ostringstream s;
      if (RefBase::sSequence[i]->PutHowto(s)) {
        DecrementOrigin(11);
        s << ".";
        DrawLabel(XYPt(0), s.str(), LABELSTYLE_NORMAL);
      }
    }
  }
  
  // Close the file.  
  (*mStream) << "showpage" << endl;
  (*mStream) << "%%Trailer" << endl;
  (*mStream) << "%%Pages: " << mPSPageCount << endl;
}


/*****
Write the PostScript code that draws folding sequences for a list of marks.
*****/

void PSStreamDgmr::PutMarkList(const XYPt& pp, vector<RefMark*>& vm)
{
  PutRefList(pp, vm);
}


/*****
Write the PostScript code that draws folding sequences for a list of lines.
*****/
void PSStreamDgmr::PutLineList(const XYLine& ll, vector<RefLine*>& vl)
{
  PutRefList(ll, vl);
}
