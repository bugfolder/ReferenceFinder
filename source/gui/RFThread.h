/******************************************************************************
File:         RFThread.h
Project:      ReferenceFinder 4.x
Purpose:      Header for database initialization thread
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-27
Copyright:    ©1999-2006 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _RFTHREAD_H_
#define _RFTHREAD_H_

#include "RFPrefix.h"

#include "ReferenceFinder.h"

#include "wx/thread.h"

#if !wxUSE_THREADS
    #error "This class requires thread support"
#endif // wxUSE_THREADS

/**********
class RFThread - base class for database threads, sharing common mutex
**********/
class RFThread : public wxThread {
public:
  typedef ReferenceFinder::DatabaseInfo DatabaseInfo;
  typedef ReferenceFinder::StatisticsInfo StatisticsInfo;

  // Thread-safe getters for status info
  static DatabaseInfo GetDatabaseInfo() {
    wxMutexLocker lock(sMutex); return sDatabaseInfo;
  };
  static StatisticsInfo GetStatisticsInfo() {
    wxMutexLocker lock(sMutex); return sStatisticsInfo;
  };
  static bool IsReady() {
    wxMutexLocker lock(sMutex); return 
      sDatabaseInfo.mStatus == ReferenceFinder::DATABASE_READY &&
      sStatisticsInfo.mStatus == ReferenceFinder::STATISTICS_DONE;
  };
  static bool IsWorking() {
    wxMutexLocker lock(sMutex); return 
      (sDatabaseInfo.mStatus != ReferenceFinder::DATABASE_EMPTY &&
        sDatabaseInfo.mStatus != ReferenceFinder::DATABASE_READY) ||
      sStatisticsInfo.mStatus != ReferenceFinder::STATISTICS_DONE;
  };
  static void SetHaltFlag(bool f) {
    wxMutexLocker lock(sMutex); sHaltFlag = f;
  };
  static bool GetHaltFlag() {
    wxMutexLocker lock(sMutex); return sHaltFlag;
  };
  
protected:
  static wxMutex sMutex;
  static DatabaseInfo sDatabaseInfo;
  static StatisticsInfo sStatisticsInfo;
  static bool sHaltFlag;
};


/**********
class RFDatabaseThread - a separate thread to build the database
**********/
class RFDatabaseThread : public RFThread {
public:
  // Threaded commands
  static void DoStartDatabase();
  static void DoHaltDatabase();
  
  // Thread-safe getters
  static bool IsWorking() {
    wxMutexLocker lock(sMutex); return 
      (sDatabaseInfo.mStatus != ReferenceFinder::DATABASE_EMPTY &&
        sDatabaseInfo.mStatus != ReferenceFinder::DATABASE_READY);
  };

private:
  // Thread implementation
  virtual void* Entry();
  virtual void OnExit();

  // We're the only entity that gets to alter the status block
  static void SetDatabaseInfo(const DatabaseInfo& info) {
    wxMutexLocker lock(sMutex); sDatabaseInfo = info;
  };
  // callback used by database to update status and check for halting
  static void DatabaseFn(ReferenceFinder::DatabaseInfo info, void* userData, 
    bool& haltFlag);
};


/**********
class RFStatisticsThread - a separate thread to calculate statistics
**********/
class RFStatisticsThread : public RFThread {
public:
  // Threaded commands
  static void DoStartStatistics();
  static void DoHaltStatistics();

  // Thread-safe getters
  static bool IsWorking() {
    wxMutexLocker lock(sMutex); return 
      sStatisticsInfo.mStatus != ReferenceFinder::STATISTICS_DONE;
  };

private:
  // Thread implementation
  virtual void* Entry();
  virtual void OnExit();

  // We're the only entity that gets to alter the status block
  static void SetStatisticsInfo(const StatisticsInfo& info) {
    wxMutexLocker lock(sMutex); sStatisticsInfo = info;
  };
  // callback used by database to update status and check for halting
  static void StatisticsFn(ReferenceFinder::StatisticsInfo info, void* userData, 
    bool& haltFlag);
};


#endif // _RFTHREAD_H_
