/******************************************************************************
File:         ReferenceFinder_console.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for command-line version of ReferenceFinder
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-22
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "ReferenceFinder.h"
#include "RFVersion.h"

#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

// #define CALCINPUT to 1 to use the expression evaluator, which accepts
// #symbolic input. If  CALCINPUT is 0, we'll just use the standard console
// #cin, which wants to see decimal values.
#ifndef CALCINPUT
  #define CALCINPUT 1
#endif


#if CALCINPUT
#include "parser.h"
Parser parser;
/*****
Get a number from the user via the expression evaluator
*****/
static void ReadNumber(double& n, bool assumeDefault = true)
{
  // Read a string from console, parse a numerical expression and save result
  // in parameter. Loop until a valid expression is entered. Abort if user ends
  // input. At present, double ReadNumber (void) would be enough; we could
  // generalize it to boolean ReadNumber (double &) and let user abort without
  // entering a valid expression.
  string buffer;  // text read here
  for ( ; ! cin.eof (); ) {
    getline (cin, buffer);
    Parser::Status st = parser.evaluate (buffer, n, assumeDefault);
    if (! st.isOK ())
      cerr << "  Error (" << st << "), try again: ";
    else
      break;
  }
  if (cin.eof ())
    exit (0);
}
#else
/*****
Get a number from the user via standard input cin.
*****/
static void ReadNumber (double& n, bool) {
  cin >> n;
}
#endif // CALCINPUT


/*****
Callback routine to show progress by reporting information to the console.
*****/
void ConsoleDatabaseProgress(ReferenceFinder::DatabaseInfo info, void*, bool&)
{
  switch (info.mStatus) {
    case ReferenceFinder::DATABASE_INITIALIZING:
      // Called at beginning of initialization
      cout << "Initializing using";
      if (ReferenceFinder::sUseRefLine_C2P_C2P) cout << " O1,";
      if (ReferenceFinder::sUseRefLine_P2P) cout << " O2,";
      if (ReferenceFinder::sUseRefLine_L2L) cout << " O3,";
      if (ReferenceFinder::sUseRefLine_L2L_C2P) cout << " O4,";
      if (ReferenceFinder::sUseRefLine_P2L_C2P) cout << " O5,";
      if (ReferenceFinder::sUseRefLine_P2L_P2L) cout << " O6,";
      if (ReferenceFinder::sUseRefLine_L2L_P2L) cout << " O7,";
      cout << " vis=";
      if (ReferenceFinder::sVisibilityMatters) cout << "true";
      else cout << "false";
      cout << " wce=";
      if (ReferenceFinder::sLineWorstCaseError) cout << "true";
      else cout << "false";
      cout << flush;  
      break;
      
    case ReferenceFinder::DATABASE_WORKING:
      // Called while we're building lines and marks
      cout << "." << flush;
      break;
    
    case ReferenceFinder::DATABASE_RANK_COMPLETE:
      // Called when we've finished a rank
      cout << endl << "There are " << 
        info.mNumLines << " lines and " << 
        info.mNumMarks << " marks of rank <= " << 
        info.mRank << " " << flush;
      break;
    
    case ReferenceFinder::DATABASE_READY:
      // Called when we're completely done
      cout << endl << endl << flush;
      break;
  }
}


/*****
Callback routine for statistics function
*****/
void ConsoleStatisticsProgress(ReferenceFinder::StatisticsInfo info, void*, bool&)
{
  switch (info.mStatus) {
    case ReferenceFinder::STATISTICS_BEGIN: {
      // Initialize reporting of each trial
      cout.precision(4);
      cout.setf(ios_base::fixed, ios_base::floatfield);
      cout << "(test #) error" << endl;
      break;
    }
    case ReferenceFinder::STATISTICS_WORKING: {
      // Print the trials, 5 to a line
      cout << "(" << info.mIndex + 1 << "/" << ReferenceFinder::sNumTrials << 
        ") " << info.mError << ", ";
      if (fmod(double(info.mIndex) + 1, 5) == 0) cout << endl;
      break;
    }
    case ReferenceFinder::STATISTICS_DONE: {
      // Report the results
      cout << endl;
      cout << ReferenceFinder::sStatistics << endl;
      cout << endl;
      break;
    }
  }
}


/*****
Open a new file stream for output, sequentially numbered
*****/
void OpenPSFile(string& fileName, fstream& fout)
{
  static int fileCount = 0;
  stringstream fileNameStr;
  fileNameStr << "ReferenceFinder_" << setw(3) << setfill('0') << 
    ++fileCount << ".ps";
  fileName = fileNameStr.str();
  fout.open(fileName.c_str(), ios::out | ios::trunc);
  if (!fout.good()) {
    cout << "error opening file" << endl;
    return;
  }
}


/******************************
Main program loop
******************************/
int main()
{ 
  cout << APP_V_M_B_NAME_STR << " (build " << BUILD_CODE_STR << ")" << endl;
  cout << "Copyright (c)1999-2006 by Robert J. Lang. All rights reserved." << endl;
  
  VerbalStreamDgmr vsdgmr(cout);
  ReferenceFinder::SetDatabaseFn(&ConsoleDatabaseProgress);
  ReferenceFinder::SetStatisticsFn(&ConsoleStatisticsProgress);
  ReferenceFinder::MakeAllMarksAndLines();

  //  Loop forever until the user quits from the menu.
  while (true) {
    cout << "0 = exit, 1 = find mark, 2 = find line : ";
    double ns;
    ReadNumber(ns, false);

    switch (int(ns)) {
      case 0: {
        exit(1);
        break;
      }
      case 1: {
        XYPt pp(0, 0);  
        cout << endl << "Enter x coordinate: ";
        ReadNumber (pp.x);
        cout << "Enter y coordinate: ";
        ReadNumber (pp.y);
        string err;
        if (ReferenceFinder::ValidateMark(pp, err)) {
          vector<RefMark*> vm;
          ReferenceFinder::FindBestMarks(pp, vm, 5);
          
          // Write verbal directions to the console
          vsdgmr.PutMarkList(pp, vm);
          
          // Also draw Postscript directions to a file
          string fileName;
          fstream fout;
          OpenPSFile(fileName, fout);
          PSStreamDgmr pdgmr(fout);
          pdgmr.PutMarkList(pp, vm);
          fout.close();
          cout << "Diagrams in <" << fileName << ">." << endl;
        }
        else 
          cout << err << endl;
        break;
      }
      case 2: {
        XYPt p1, p2;
        cout << endl << "Enter p1 x coordinate: ";
        ReadNumber (p1.x);
        cout << "Enter p1 y coordinate: ";
        ReadNumber (p1.y);
        cout << endl << "Enter p2 x coordinate: ";
        ReadNumber (p2.x);
        cout << "Enter p2 y coordinate: ";
        ReadNumber (p2.y);
        string err;
        if (ReferenceFinder::ValidateLine(p1, p2, err)) {
          XYLine ll(p1, p2);
          vector<RefLine*> vl;
          ReferenceFinder::FindBestLines(ll, vl, 5);
          
          // Write verbal directions to the console
          vsdgmr.PutLineList(ll, vl);
          
          // Also draw Postscript directions to a file
          string fileName;
          fstream fout;
          OpenPSFile(fileName, fout);
          PSStreamDgmr pdgmr(fout);
          fout.close();
          cout << "Diagrams in <" << fileName << ">." << endl;
        }
        else 
          cout << err << endl;
        break;      
      }
      case 99: {
        // hidden command to calculate statistics on marks & report results
        ReferenceFinder::CalcStatistics();
        break;
      }
      default:
        cout << "Enter just 0, 1 or 2, please.\n\n";
    }
  };
  return 0;
}
