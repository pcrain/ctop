#include "ptop.h"

namespace ptop {

unsigned _sleep_usecs = 1000000;
bool shouldExit = false;
std::map<int,std::string> userIdMap;
int uptime;

int run(int argc, char** argv) {
  int flags, opt;
  while ((opt = getopt(argc, argv, "t:")) != -1) {
      switch (opt) {
      case 't':
        _sleep_usecs = atof(optarg)*1000000;
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-t nsecs]\n", argv[0]);
        exit(-1);
      }
  }

  int error  = 0;
  shouldExit = false;

  init_exit_handler();
  init_curses();

  std::thread *tcpdumpThread = new std::thread(ptop::tcpdump);
  eventLoop();
  tcpdumpThread->join();
  delete tcpdumpThread;

  exit_normal();
  return 0;
}

int eventLoop() {
  int                                  pid;
  std::string                          path = "/proc";
  std::map<int,ProcessData*>           pidmap;
  std::map<int,ProcessData*>::iterator piditerator;
  bool kernelthreads[MAX_PID] = { false };
  std::vector<ProcessData*>            sortedpids;
  ProcessData* curProcess;
  ProcessFields                        lastsortmode, cursortmode;
  bool                                 lastaverageusage = false;

  generateUserIdMap();
  ProcessData::initProcessDataClass();

  ProcessData::setProcessSortMode(ProcessFields::age);
  // ProcessData::setProcessSortMode(ProcessFields::pid);
  // ProcessData::setProcessSortMode(ProcessFields::command);

  cursortmode = ProcessData::getProcessSortMode();
  lastsortmode = ProcessFields::END;

  DIR *d;
  struct dirent *dir;

  while (!shouldExit) {
    unsigned counter = 0;

    uptime = stoi(splitAndFind(readAllLines("/proc/uptime"),".")[0]);

    d = opendir("/proc");
    while ((dir = readdir(d)) != NULL) {
      if (shouldExit) {
        return 1;
      }
      char firstChar = dir->d_name[0];
      if (firstChar < '0' || firstChar > '9') {
        continue; //Not a PID
      }
      std::string pidpath = dir->d_name;
      pid = std::stoi(pidpath);  //Shouldn't need a try block; nothing in /proc should begin with a digit other than a pid
      if (kernelthreads[pid]) {
        continue;  //Kernel thread
      }
      if (access(("/proc/"+pidpath+"/cmdline").c_str(), R_OK) != 0) {
        continue;  //We do not have read access to this process
      }
      piditerator = pidmap.find(pid);
      if (piditerator == pidmap.end()) {
        std::string cmdline = readAllLines(("/proc/"+pidpath+"/cmdline").c_str(),' ').substr(0,MAX_CMD_LENGTH);
        if (cmdline.length() == 0) {
          kernelthreads[pid] = true;
          continue;
        }
        curProcess = new ProcessData(pid,cmdline);
        pidmap[pid] = curProcess;
        curProcess->setIsNew(true);
      } else {
        curProcess = pidmap[pid];
        curProcess->setIsNew(false);
      }
      curProcess->setStillRunning(true);
    }
    closedir(d);

    sortedpids.clear();
    for (piditerator = pidmap.begin(); piditerator != pidmap.end(); ) {
      if (! piditerator->second->checkIfStillRunning()) {
        delete piditerator->second;
        piditerator = pidmap.erase(piditerator); //TODO: memory leak?
        continue;
      }
      piditerator->second->setStillRunning(false); //Gets refreshed next cycle if it is in fact still running
      sortedpids.push_back(piditerator->second);
      ++piditerator;
    }
    for (unsigned i = 0; i < sortedpids.size(); ++i) {
      sortedpids[i]->updateData();
    }
    cursortmode = ProcessData::getProcessSortMode();
    if ((cursortmode != lastsortmode) || (ProcessData::_showAverageUsage != lastaverageusage)) {
      lastsortmode = cursortmode;
      lastaverageusage = ProcessData::_showAverageUsage;
      writeHeader();
    }
    std::sort(sortedpids.begin(),sortedpids.end(),ProcessData::compareProcessByPointer);

    //NEW
    moveCursorToBeginningOfScreen();
    int maxy = _start_y+_term_h;
    if (maxy > (int)sortedpids.size()) {
      maxy = sortedpids.size();
    }
    for (int i = _start_y; i < maxy; ++i) { sortedpids[i]->writeData(); }

    for (unsigned i = 0; i < 10; ++i) {
      writeEmptyLine();
    }
    curreset();
    refresh_pad();
    usleep(_sleep_usecs);
  }
  return 1;
}

void writeHeader() {
  const char* const headers[ProcessFields::END] = {
        "  PID",    // pid,
    "     USER", // user,
          " PR",      // priority,
           " S",        // status,
      "   TIME",   // cputime,
         " AGE",      // age,
       "   RAM",    // ram,
      "   CPU%",   // cpu,
      " AVCPU%",   // cpuavg,
       "  RD/s",    // read,
       "  WT/s",    // write,
       "  UL/s",    // upload,
       "  DL/s",    // download,
      "    TTY",   // tty,
    //     " SPWN",     // spawner,
    // " XRWVAIDC", // flags,
        " CMD "       // command,
  };
  const char* const avgheaders[ProcessFields::END] = {
        "  PID",    // pid,
    "     USER", // user,
          " PR",      // priority,
           " S",        // status,
      "   TIME",   // cputime,
         " AGE",      // age,
       "   RAM",    // ram,
      "   CPU%",   // cpu,
      " AVCPU%",   // cpuavg,
       "  RDAV",    // read,
       "  WTAV",    // write,
       "  ULAV",    // upload,
       "  DLAV",    // download,
      "    TTY",   // tty,
    //     " SPWN",     // spawner,
    // " XRWVAIDC", // flags,
        " CMD "       // command,
  };
  for (unsigned i = 0; i < ProcessFields::END; ++i) {
    curprintDirect(
      A_BOLD | A_STANDOUT | ((i == ProcessData::_process_sort_mode) ? COLOR_PAIR(col::RED) : 0),
      ProcessData::_showAverageUsage ? avgheaders[i] : headers[i]
      );
  }
  curprintDirect("\n");
  return;
}

void generateUserIdMap() {
  std::string contents = readAllLines("/etc/passwd");
  std::istringstream iss(contents);

  std::regex regex_userentry("(.*?):(.*?):([0-9]+)");
  std::smatch sm;

  for (std::string line; std::getline(iss, line); ) {
    std::regex_search (line,sm,regex_userentry);
    userIdMap[stoi(sm[3])] = sm[1];
  }
}

std::string lookupUser(int uid) {
  return userIdMap[uid];
}

void exit_normal() {
  end_curses();
}

void exit_handler(int s) {
  shouldExit = true;
  return;
  // end_curses();
  // exit(0);
}

void init_exit_handler() {
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = exit_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, NULL);
}

}
