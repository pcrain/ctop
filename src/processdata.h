#ifndef PROCESS_H_
#define PROCESS_H_

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <exception>
#include <regex>
#include <limits.h>

#include "termoutput.h"
#include "util.h"

namespace ptop {

// #define NOW std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

const int MAX_CMD_LENGTH = 500;
const int CLOCK_TICKS = 100;
const unsigned _STAT_BUFFER_SIZE = 100;
const unsigned PAGESIZE_KB = 4; //$ getconf PAGESIZE

extern bool shouldExit;
extern std::map<int,std::string> userIdMap;
extern int uptime;

enum ProcessFields {
  pid,
  user,
  priority,
  status,
  cputime,
  age,
  ram,
  cpu,
  cpuavg,
  read,
  write,
  upload,
  download,
  tty,
  // spawner,
  // flags,
  command,
  END
};

struct CursesEntry {
  std::string value;  //String we're printing
  int         flags;  //Flags we're printing with
};

class ProcessData;

typedef void (ptop::ProcessData::*VoidFunc)(void);
typedef CursesEntry (ptop::ProcessData::*CurseFunc)(void);

class ProcessData {
private:
  unsigned    _pid;      //PID of the process
  std::string _path;     //Path to the process' directory
  std::string _cmdline;  //Command line for the currently running process
  std::string _owner;    //Owner of the PID
  std::string _tty;      //TTY the process was launched in
  std::string _spawner;  //Method by which the process was spawned
  int         _userid;
  std::string _username;
  char        _status;   //Process status
  int         _priority; //Priority of the process
  float       _cputime;  //CPU usage-seconds of the process
  int         _launchtime;
  float       _runtime;    //Number of seconds of cpu time the process has used
  float       _lastruntime;
  int         _age;      //Number of seconds since the process was launched
  int         _ram;
  float       _cpuusage;
  float       _avgcpuusage;

  //Cached CursesEntries
  std::vector<CursesEntry> _fcmdline;
  CursesEntry _ce_tty;
  CursesEntry _ce_pid;
  CursesEntry _ce_user;

  long int         _readbytes;
  long int         _writebytes;
  long int         _uploadbytes;
  long int         _downloadbytes;

  long int         _lastreadbytes;
  long int         _lastwritebytes;
  long int         _lastuploadbytes;
  long int         _lastdownloadbytes;

  float       _readdelta;
  float       _writedelta;
  float       _uploaddelta;
  float       _downloaddelta;

  long long int _lastupdateusecs;
  long long int _curupdateusecs;
  long long int _usecssincelastupdate;
  float         _updatedelta;

  // std::vector<std::string> _statBuffer;

  std::string  _statBuffer[_STAT_BUFFER_SIZE];
  unsigned     _statBufferLength;
  char*        _statBufferArray;


  bool        _isNew;
  bool        _stillRunning;
  // CursesEntry (*_printFunctions)()[ProcessFields.END];

  void formatCommandLine();
public:
  static bool          _showAverageUsage;
  static ProcessFields _process_sort_mode;
  static VoidFunc      _updateFunctions[ProcessFields::END];
  static CurseFunc     _writeFunctions[ProcessFields::END];

  ProcessData(int pid, std::string cmdline);
  ~ProcessData();
  bool successFullyLoaded();
  const char* getCommandLine();
  void updateData();
  void writeData();

  void updateStat();
  void updateRam();
  void updateCpu();
  void updateAverageCpu();
  void updateDiskIO();
  void updateNetIO();

  CursesEntry writePriority();
  CursesEntry writePid();
  // CursesEntry writeCommandLine();
  CursesEntry writeUser();
  CursesEntry writeTty();
  CursesEntry writeStatus();
  CursesEntry writeAge();
  CursesEntry writeCpuTime();
  CursesEntry writeRam();
  CursesEntry writeCpu();
  CursesEntry writeAverageCpu();
  CursesEntry writeRead();
  CursesEntry writeWrite();
  CursesEntry writeUpload();
  CursesEntry writeDownload();

  static void initProcessDataClass();

  inline void setIsNew(bool b) { _isNew = b; }
  inline bool getIsNew() { return _isNew; }

  inline void doNothing() {
    return;
  };

  inline CursesEntry writeNothing() {
    return { "", 0 };
  };

  inline bool checkIfStillRunning() {
    return _stillRunning;
  }

  inline void setStillRunning(bool v) {
    _stillRunning = v;
  }

  static inline void setProcessSortMode(ProcessFields mode) {
    ProcessData::_process_sort_mode = mode;
  }

  static inline ProcessFields getProcessSortMode() {
    return ProcessData::_process_sort_mode;
  }

  // static bool processComparator (const ProcessData *c1, const ProcessData *c2) {
  //   switch(ProcessData::_process_sort_mode) {
  //     case ProcessFields::command:
  //       return c1->_cmdline < c2->_cmdline;
  //     case ProcessFields::pid:
  //       return c1->_pid < c2->_pid;
  //     default:
  //       return c1->_pid < c2->_pid;
  //   }
  // }

  inline bool operator< (const ProcessData &other) {
    switch(ProcessData::_process_sort_mode) {
      case ProcessFields::command: return _cmdline.compare(other._cmdline) < 0;
      case ProcessFields::age:     return _age < other._age;
      case ProcessFields::cpuavg:  return _avgcpuusage > other._avgcpuusage;
      case ProcessFields::cputime: return _runtime > other._runtime;
      case ProcessFields::cpu:     return _cpuusage > other._cpuusage;
      case ProcessFields::user:    return _username < other._username;
      case ProcessFields::priority:return _priority < other._priority;
      case ProcessFields::status:  return _status > other._status;
      case ProcessFields::ram:     return _ram > other._ram;
      case ProcessFields::read:    return _readdelta > other._readdelta;
      case ProcessFields::write:   return _writedelta > other._writedelta;
      case ProcessFields::upload:  return _uploaddelta > other._uploaddelta;
      case ProcessFields::download:return _downloaddelta > other._downloaddelta;
      case ProcessFields::tty:     return _tty.compare(other._tty) > 0;
      case ProcessFields::pid:     return _pid < other._pid;
      default:                     return _pid < other._pid;
    }
  }

  static inline bool compareProcessByPointer (ProcessData *a, ProcessData *b) {
    return (*a) < (*b);
  }

};

}

#endif /* PROCESS_H_ */
