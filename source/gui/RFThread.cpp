/******************************************************************************
File:         RFDatabaseThread.cpp
Project:      ReferenceFinder 4.x
Purpose:      Implementation for database initialization thread
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-24
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#include "RFThread.h"

#include <sstream>

using namespace std;

/* Notes on RFThread
Calculation of the database and calculation of statistics are both long
operations, so we carry both sets of calculations out in separate threads. The
database is protected by a common mutex, which is a static variable of the base
class.

ReferenceFinder accepts a callback function that gets polled periodically
during MakeAllMarksAndLines() and CalcStatistics(); we use this callback
mechanism to update a DatabaseInfo block that the application can check to
enable/disable menu commands and decide what to display, and to halt rebuild if
we get tired of waiting. Since this status info is shared between the main
(GUI) thread and our secondary rebuild thread, we use a mutex to protect the
shared info.

Other commands into the database are not bottlenecked through the thread
interface, so we have to check database status using IsWorking() and/or
IsReady() before issuing such commands.
*/

/**********
class RFThread - base class for database threads, sharing common mutex
**********/

/*****
Static variables
*****/
wxMutex RFThread::sMutex;
RFThread::DatabaseInfo RFThread::sDatabaseInfo;
RFThread::StatisticsInfo RFThread::sStatisticsInfo;
bool RFThread::sHaltFlag;


#ifdef __MWERKS__
#pragma mark -
#endif


/* Notes on RFDatabaseThread.
Rebuilding the ReferenceFinder database (i.e., building a listing of all
references in the square) takes a long time. To avoid tying up the UI during
rebuild, we wrap the call to ReferenceFinder::MakeAllMarksAndLines() in a
separate thread. ReferenceFinder accepts a callback function that gets polled
periodically during database construction; we use this callback to update a
DatabaseInfo block that the application can check to enable/disable menu commands
and decide what to display, and to halt rebuild if we get tired of waiting.
Since this status info is shared between the main (GUI) thread and our
secondary rebuild thread, we use a mutex to protect the DatabaseInfo block.
*/

/**********
class RFDatabaseThread - a separate thread to build the database
**********/

/*****
Rebuild the database
*****/
void RFDatabaseThread::DoStartDatabase()
{
  DoHaltDatabase();
  RFDatabaseThread* thread = new RFDatabaseThread();
#ifdef RFDEBUG
  bool success = 
#endif // RFDEBUG
    (thread->Create() == wxTHREAD_NO_ERROR);
  RFASSERT(success);
  thread->Run();
}


/*****
Halt rebuild if it's going on. It is safe to call this even if the
database isn't currently building.
*****/
void RFDatabaseThread::DoHaltDatabase()
{
  if (IsWorking()) {
    // The database is currently rebuilding; which means we should terminate
    // the existing rebuild before we restart a new one.
    SetHaltFlag(true);
    // Now we have no choice but to wait until the database thread sees that
    // we've set the halt flag and terminates.
    for (;;) {
      // Kill some time in the calling thread (the main GUI thread) so that the
      // database thread has time to see that we've set the halt flag and bring
      // its calculation to a clean finish.
      wxMilliSleep(10);
      if (!IsWorking()) break;
    }
    // Now, it's remotely possible that initialization actually halted on its
    // own between the call to IsWorking() and the first SetHaltFlag(true) call,
    // which would have left the halt flag on; so now that we're sure it's
    // halted, we'll make sure the flag is off.
    SetHaltFlag(false);
  }
}


/*****
Entry to the thread: initialize the database.
*****/
void* RFDatabaseThread::Entry()
{
  ReferenceFinder::SetDatabaseFn(&DatabaseFn, this);
  ReferenceFinder::MakeAllMarksAndLines();
  return 0;
}


/*****
Note that we've halted
*****/
void RFDatabaseThread::OnExit()
{
  SetHaltFlag(false);
}


/*****
Callback function from ReferenceFinder database, which we use to update the
DatabaseInfo block in our application and to test for halting.
******/
void RFDatabaseThread::DatabaseFn(ReferenceFinder::DatabaseInfo info, 
  void* userData, bool& haltFlag)
{
  // Required check for termination of thread
  RFASSERT(userData);
  RFDatabaseThread* thread = (RFDatabaseThread*) userData;
  if (thread->TestDestroy()) return;
  
  // Write the status info to our local variable and if our halt flag has been
  // set, set the haltFlag flag in the callback for the database.
  SetDatabaseInfo(info);
  if (GetHaltFlag()) haltFlag = true;
}


#ifdef __MWERKS__
#pragma mark -
#endif


/**********
class RFStatisticsThread - a separate thread to calculate database statistics
**********/

/*****
Rebuild the database
*****/
void RFStatisticsThread::DoStartStatistics()
{
  DoHaltStatistics();
  RFStatisticsThread* thread = new RFStatisticsThread();
#ifdef RFDEBUG
  bool success = 
#endif // RFDEBUG
    (thread->Create() == wxTHREAD_NO_ERROR);
  RFASSERT(success);
  thread->Run();
}


/*****
Halt analysis of test cases.
*****/
void RFStatisticsThread::DoHaltStatistics()
{
  if (IsWorking()) {
    SetHaltFlag(true);
    for (;;) {
      wxMilliSleep(10);
      if (!IsWorking()) break;
    }
    SetHaltFlag(false);
  }
}


/*****
Entry to the thread: calculate statistical information.
*****/
void* RFStatisticsThread::Entry()
{
  ReferenceFinder::SetStatisticsFn(&StatisticsFn, this);
  stringstream ss;
  ReferenceFinder::CalcStatistics();
  return 0;
}


/*****
Note that we've halted
*****/
void RFStatisticsThread::OnExit()
{
  SetHaltFlag(false);
}


/*****
Callback function from ReferenceFinder database, which we use to update the
StatisticsInfo block in our application and to test for halting.
******/
void RFStatisticsThread::StatisticsFn(ReferenceFinder::StatisticsInfo info, 
  void* userData, bool& haltFlag)
{
  // Required check for termination of thread
  RFASSERT(userData);
  RFStatisticsThread* thread = (RFStatisticsThread*) userData;
  if (thread->TestDestroy()) return;
  
  // Write the status info to our local variable and if our halt flag has been
  // set, set the haltFlag flag in the callback for the database.
  SetStatisticsInfo(info);
  if (GetHaltFlag()) haltFlag = true;
}
