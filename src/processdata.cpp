#include "processdata.h"

namespace ptop {

bool          ProcessData::_showAverageUsage  = false;
ProcessFields ProcessData::_process_sort_mode = ProcessFields::pid;
VoidFunc      ProcessData::_updateFunctions[ProcessFields::END];
CurseFunc     ProcessData::_writeFunctions[ProcessFields::END];

//Macro for reading an unisnged integer into "istorage" from "carray" starting at "acc" and ending at "delim"
#define INTFROMCHARARRAY(istorage,carray,acc,delim) \
  istorage = 0;                                     \
  while(carray[++acc] != (delim)) {                 \
    istorage *= 10;                                 \
    istorage += carray[acc]-'0';                    \
  }                                                 \
  --acc; //Put it back to the space before

ProcessData::ProcessData(int pid, std::string cmdline) {
  static std::regex regex_uid("Uid:\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)");
  static std::smatch sm;

  _pid     = pid;
  _path    = "/proc/"+std::to_string(pid);
  _cmdline = cmdline;
  formatCommandLine();

  //Defaults
 _priority             = 20;
 _status               = '?';
 _launchtime           = 0;
 _age                  = 0;
 _ram                  = -1;
 _lastruntime          = -1;
 _runtime              = -1;
 _updatedelta          = 1000000000;
 _avgcpuusage          = 0;
 _readdelta            = 0;
 _writedelta           = 0;
 _uploaddelta          = 0;
 _downloaddelta        = 0;
 _readbytes            = 0;
 _writebytes           = 0;
 _uploadbytes          = 0;
 _downloadbytes        = 0;
 _lastreadbytes        = -1;
 _lastwritebytes       = -1;
 _lastuploadbytes      = -1;
 _lastdownloadbytes    = -1;
 _curupdateusecs       = getTimeInUsecs();
 _lastupdateusecs      = _curupdateusecs;
 _usecssincelastupdate = 0;
 _statBufferArray      = nullptr;

  std::string statfile = readAllLines(_path+"/status");
  std::regex_search (statfile,sm,regex_uid);
  try {
    _userid = stoi(sm[2]);
    _username = lookupUser(_userid);
  } catch (const std::invalid_argument& ia) {
    _userid = -1;
    _username = "???";
  }

  //Get TTY
  try {
    char* ttypath = realpath((_path+"/fd/0").c_str(),nullptr);
    _tty = std::string(ttypath);
    size_t pos;
    pos = _tty.find("/dev/tty");
    if (pos != std::string::npos) {
      _ce_tty = { format("%6s ",_tty.substr(5).c_str()), COLOR_PAIR(col::GRN) };
    } else {
      pos = _tty.find("/dev/pts/");
      if (pos != std::string::npos) {
        _ce_tty = { format("%6s ",("pts"+_tty.substr(9)).c_str()), COLOR_PAIR(col::CYN) };
      } else {
        pos = _tty.find("/dev/null");
        if (pos != std::string::npos) {
          _ce_tty = { format("%6s ","null"), A_BOLD | COLOR_PAIR(col::BLK) };
        } else {
          _ce_tty = { format("%6s "," "), 0 };
        }
      }
    }
  } catch (const std::logic_error& le) {
    _tty = "???";
    _ce_tty = { format("%6s "," "), 0 };
  }

  _ce_pid = { format("%5i ",this->_pid), COLOR_PAIR(col::WHT) };
  col color =
    this->_userid == 0              ? col::RED :
    this->_userid == (int)geteuid() ? col::GRN :
    this->_userid < 1000            ? col::CYN : col::WHT ;
  _ce_user = { format("%8s ",this->_username.c_str()), COLOR_PAIR(color) };

  _stillRunning = true;
}

void ProcessData::initProcessDataClass() {
  for (unsigned i = 0; i < ProcessFields::END; ++i) {
    ProcessData::_updateFunctions[i] = nullptr;
    ProcessData::_writeFunctions[i]  = &ProcessData::writeNothing;
  }

  ProcessData::_writeFunctions[ProcessFields::pid]         = &ProcessData::writePid;
  // ProcessData::_writeFunctions[ProcessFields::command]     = &ProcessData::writeCommandLine;
  ProcessData::_writeFunctions[ProcessFields::user]        = &ProcessData::writeUser;
  ProcessData::_writeFunctions[ProcessFields::tty]         = &ProcessData::writeTty;

  ProcessData::_updateFunctions[ProcessFields::priority]   = &ProcessData::updateStat;
  ProcessData::_writeFunctions[ProcessFields::priority]    = &ProcessData::writePriority;
  ProcessData::_writeFunctions[ProcessFields::status]      = &ProcessData::writeStatus;
  ProcessData::_writeFunctions[ProcessFields::age]         = &ProcessData::writeAge;
  ProcessData::_writeFunctions[ProcessFields::cputime]     = &ProcessData::writeCpuTime;

  ProcessData::_updateFunctions[ProcessFields::ram]        = &ProcessData::updateRam;
  ProcessData::_writeFunctions[ProcessFields::ram]         = &ProcessData::writeRam;

  ProcessData::_updateFunctions[ProcessFields::cpu]        = &ProcessData::updateCpu;
  ProcessData::_writeFunctions[ProcessFields::cpu]         = &ProcessData::writeCpu;

  ProcessData::_updateFunctions[ProcessFields::cpuavg]     = &ProcessData::updateAverageCpu;
  ProcessData::_writeFunctions[ProcessFields::cpuavg]      = &ProcessData::writeAverageCpu;

  ProcessData::_updateFunctions[ProcessFields::read]       = &ProcessData::updateDiskIO;
  ProcessData::_writeFunctions[ProcessFields::read]        = &ProcessData::writeRead;
  ProcessData::_writeFunctions[ProcessFields::write]       = &ProcessData::writeWrite;

  ProcessData::_updateFunctions[ProcessFields::upload]     = &ProcessData::updateNetIO;
  ProcessData::_writeFunctions[ProcessFields::upload]      = &ProcessData::writeUpload;
  ProcessData::_writeFunctions[ProcessFields::download]    = &ProcessData::writeDownload;
}

const char* ProcessData::getCommandLine() {
  return _cmdline.c_str();
}

bool ProcessData::successFullyLoaded() {
  if (_cmdline.length() < 1) {
    return false;
  }
  return true;
}

CursesEntry ProcessData::writePid() {
  return _ce_pid;
}
CursesEntry ProcessData::writeUser() {
  return _ce_user;
}
CursesEntry ProcessData::writeTty() {
  return _ce_tty;
}

//NOTE: this function updatess process state, run time, priority, and launch time
void ProcessData::updateStat() {
  //If we can't read stat, we're screwed
  long length;
  if (! readProcLines((_path+"/stat").c_str(),&_statBufferArray,&length)) {
    return;
  }
  //Skip past first right parenthesis
  unsigned pos;
  for(pos = 0; _statBufferArray[pos] != ')' && pos < length; ++pos);

  int uruntime      = 0;
  int kruntime      = 0;
  unsigned spacenum = 0;
  bool alldone      = false;
  for(;(!alldone) && (pos < length);++pos) {
    if (_statBufferArray[pos] == ' ') {
      ++spacenum;
      switch(spacenum) {
        case 1:  //Status of process
          _status = _statBufferArray[++pos];
          break;
        case 12: //User runtime
          INTFROMCHARARRAY(uruntime,_statBufferArray,pos,' ')
          break;
        case 13: //Kernel runtime
          INTFROMCHARARRAY(kruntime,_statBufferArray,pos,' ')
          if (_lastruntime > -1) {
           _lastruntime = _runtime;
          }
          _runtime = (float)(uruntime+kruntime)/CLOCK_TICKS;
          if (_lastruntime == -1) {
           _lastruntime = _runtime;
          }
          break;
        case 16:  //Priority of process
          INTFROMCHARARRAY(_priority,_statBufferArray,pos,' ')
          break;
        case 20:  //Launch time of process
          INTFROMCHARARRAY(_launchtime,_statBufferArray,pos,' ')
          _launchtime /= 100;
          _age = uptime - _launchtime;
          alldone = true; //This is the last field we care about
          break;
      }
    }
  }
  free(_statBufferArray);  //Make sure to free the memory for the statBuffer
  return;
}

void ProcessData::updateRam() {
  unsigned pos = 0;
  long length;
  if (! readProcLines((_path+"/statm").c_str(),&_statBufferArray,&length)) {
    return;
  }

  unsigned spacenum = 0;
  bool alldone = false;
  for(;(!alldone) && (pos < length);++pos) {
    if (_statBufferArray[pos] == ' ') {
      ++spacenum;
      switch(spacenum) {
        case 1:  //RAM usage of process
          INTFROMCHARARRAY(_ram,_statBufferArray,pos,' ')
          _ram *= PAGESIZE_KB;
          alldone = true; //This is the last field we care about
          break;
      }
    }
  }
  free(_statBufferArray);  //Make sure to free the memory for the statBuffer
  return;
}

CursesEntry ProcessData::writeRam() {
  col color;
  std::string f;
  if      (_ram < 10000)    { f = format("%4uK ",_ram); }
  else if (_ram < 10240000) { f = format("%4uM ",_ram/1024); }
  else                      { f = format("%4uG ",_ram/1024/1024); }

  int flags =
    _ram > 10485760 ? COLOR_PAIR(col::RED) :
    _ram > 1048576  ? COLOR_PAIR(col::YLW) :
    _ram > 102400   ? COLOR_PAIR(col::GRN) :
    _ram > 10240    ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

void ProcessData::updateCpu() {
  _cpuusage = 100 * ((_runtime - _lastruntime) / _updatedelta);
}

CursesEntry ProcessData::writeCpu() {
  std::string f = format("%6.2f ",_cpuusage);

  int flags =
    _cpuusage > 80 ? COLOR_PAIR(col::YLW) :
    _cpuusage > 10 ? COLOR_PAIR(col::GRN) :
    _cpuusage > 0  ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

void ProcessData::updateDiskIO() {
  //If we can't read stat, we're screwed
  unsigned pos = 0;
  long length;
  if (! readProcLines((_path+"/io").c_str(),&_statBufferArray,&length)) {
    return;
  }

  unsigned spacenum = 0;
  unsigned linenum = 0;
  bool alldone      = false;
  for(;(!alldone) && (pos < length);++pos) {
    if (_statBufferArray[pos] == '\n') {
      ++linenum;
      continue;
    }
    if (_statBufferArray[pos] == ':') {
      ++pos;
      switch(linenum) {
        case 0:  //Read (2 for no cache)
          INTFROMCHARARRAY(_readbytes,_statBufferArray,pos,'\n');
          if (ProcessData::_showAverageUsage) {
            _readdelta = (_age == 0) ? 0 : float(_readbytes) / _age;
          } else {
            _readdelta = float(_readbytes - _lastreadbytes) / _updatedelta;
            if (_lastreadbytes == -1) {
              _readdelta = 0;
            }
          }
          _lastreadbytes  = _readbytes;
          break;
        case 1: //Write (3 for no cache)
          INTFROMCHARARRAY(_writebytes,_statBufferArray,pos,'\n');
          if (ProcessData::_showAverageUsage) {
            _writedelta = (_age == 0) ? 0 : float(_writebytes) / _age;
          } else {
            _writedelta = float(_writebytes - _lastwritebytes) / _updatedelta;
            if (_lastwritebytes == -1) {
              _writedelta = 0;
            }
          }
          _lastwritebytes = _writebytes;
          alldone = true; //This is the last field we care about
          break;
      }
    }
  }
  free(_statBufferArray);  //Make sure to free the memory for the statBuffer
  return;
}

void ProcessData::updateNetIO() {
  _uploadbytes   = iread[std::to_string(_pid)];
  _downloadbytes = iwrite[std::to_string(_pid)];

  if (ProcessData::_showAverageUsage) {
    _uploaddelta = (_age == 0) ? 0 : float(_uploadbytes) / _age;
  } else {
    _uploaddelta   = float(_uploadbytes - _lastuploadbytes) / _updatedelta;
    if (_lastuploadbytes == -1) {
      _uploaddelta = 0;
    }
    if (_uploaddelta < 0) {
      _uploaddelta = 0;
    }
  }

  if (ProcessData::_showAverageUsage) {
    _downloaddelta = (_age == 0) ? 0 : float(_downloadbytes) / _age;
  } else {
    _downloaddelta = float(_downloadbytes - _lastdownloadbytes) / _updatedelta;
    if (_lastdownloadbytes == -1) {
      _downloaddelta = 0;
    }
    if (_downloaddelta < 0) {
      _downloaddelta = 0;
    }
  }

  _lastuploadbytes   = _uploadbytes;
  _lastdownloadbytes = _downloadbytes;
}

CursesEntry ProcessData::writeRead() {
  std::string f;
  if      (_readdelta < 1024)        { f = format("%4.0fB ",_readdelta); }
  else if (_readdelta < 1048576)     { f = format("%4.0fK ",_readdelta/1024); }
  else if (_readdelta < 1073741824)  { f = format("%4.0fM ",_readdelta/1048576); }
  else                               { f = "999+M "; }

  int flags =
    _readdelta > 1048576 ? COLOR_PAIR(col::YLW) :
    _readdelta > 1024    ? COLOR_PAIR(col::GRN) :
    _readdelta > 0       ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

CursesEntry ProcessData::writeWrite() {
  std::string f;
  if      (_writedelta < 1024)        { f = format("%4.0fB ",_writedelta); }
  else if (_writedelta < 1048576)     { f = format("%4.0fK ",_writedelta/1024); }
  else if (_writedelta < 1073741824)  { f = format("%4.0fM ",_writedelta/1048576); }
  else                                { f = "999+M "; }

  int flags =
    _writedelta > 1048576 ? COLOR_PAIR(col::YLW) :
    _writedelta > 1024    ? COLOR_PAIR(col::GRN) :
    _writedelta > 0       ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

CursesEntry ProcessData::writeUpload() {
  std::string f;
  if      (_uploaddelta < 1024)        { f = format("%4.0fB ",_uploaddelta); }
  else if (_uploaddelta < 1048576)     { f = format("%4.0fK ",_uploaddelta/1024); }
  else if (_uploaddelta < 1073741824)  { f = format("%4.0fM ",_uploaddelta/1048576); }
  else                                 { f = "999+M "; }

  int flags =
    _uploaddelta > 1048576 ? COLOR_PAIR(col::YLW) :
    _uploaddelta > 1024    ? COLOR_PAIR(col::GRN) :
    _uploaddelta > 0       ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

CursesEntry ProcessData::writeDownload() {
  std::string f;
  if      (_downloaddelta < 1024)        { f = format("%4.0fB ",_downloaddelta); }
  else if (_downloaddelta < 1048576)     { f = format("%4.0fK ",_downloaddelta/1024); }
  else if (_downloaddelta < 1073741824)  { f = format("%4.0fM ",_downloaddelta/1048576); }
  else                                   { f = "999+M "; }

  int flags =
    _downloaddelta > 1048576 ? COLOR_PAIR(col::YLW) :
    _downloaddelta > 1024    ? COLOR_PAIR(col::GRN) :
    _downloaddelta > 0       ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

void ProcessData::updateAverageCpu() {
  if (_age <= 0) {
    return;
  }
  _avgcpuusage = (100*_runtime) / _age;
}

CursesEntry ProcessData::writeAverageCpu() {
  std::string f = format("%6.2f ",_avgcpuusage);

  int flags =
    _avgcpuusage > 80 ? COLOR_PAIR(col::YLW) :
    _avgcpuusage > 10 ? COLOR_PAIR(col::GRN) :
    _avgcpuusage > 0  ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

CursesEntry ProcessData::writeStatus() {
  int flags;
  switch (this->_status) {
    case 'R': //Running
      flags = COLOR_PAIR(col::GRN); break;
    case 'S': //Interruptible sleep
      flags = A_BOLD | COLOR_PAIR(col::BLK); break;
    case 'T': //Stopped / Suspended
      flags = COLOR_PAIR(col::MGN); break;
    case 'Z': //Zombie / awaiting parent's completion
      flags = COLOR_PAIR(col::RED); break;
    case 'D': //Dormant / uninterruptible sleep
      flags = COLOR_PAIR(col::YLW); break;
    case 't': //Tracing stopped
      flags = COLOR_PAIR(col::CYN); break;
    default:
      flags = COLOR_PAIR(col::WHT); break;
  }
  return { format("%c ",this->_status), flags };
}

CursesEntry ProcessData::writeAge() {
  std::string f;
  if      (_age < 100)      { f = format("%2is ",_age); }
  else if (_age < 6000)     { f = format("%2im ",_age/60); }
  else if (_age < 360000)   { f = format("%2ih ",_age/3600); }
  else if (_age < 8640000)  { f = format("%2id ",_age/86400); }
  else if (_age < 60480000) { f = format("%2iw ",_age/604800); }
  else                      { f = format("%2iy ",_age/31536000); }

  int flags =
    _age > 3600 ? A_BOLD | COLOR_PAIR(col::BLK) :
    _age > 60   ? COLOR_PAIR(col::WHT) : COLOR_PAIR(col::GRN) ;

  return { f, flags };
}

CursesEntry ProcessData::writeCpuTime() {
  col color;
  std::string f;
  if      (_runtime < 100)    { f = format("%5.2fs ",(float)_runtime); }
  else if (_runtime < 6000)   { f = format("%5.2fm ",(float)_runtime/60); }
  else if (_runtime < 360000) { f = format("%5.2fh ",(float)_runtime/3600); }
  else                        { f = format("%5.2fd ",(float)_runtime/86400); }

  int flags =
    _runtime > 3600 ? COLOR_PAIR(col::YLW) :
    _runtime > 60   ? COLOR_PAIR(col::GRN) :
    _runtime > 0    ? COLOR_PAIR(col::WHT) : A_BOLD | COLOR_PAIR(col::BLK);
  return { f, flags };
}

CursesEntry ProcessData::writePriority() {
  int flags =
    this->_priority == 20 ? A_BOLD | COLOR_PAIR(col::BLK) :
    this->_priority > 20  ? COLOR_PAIR(col::GRN) : COLOR_PAIR(col::RED) ;
  return { format("%2i ",this->_priority), flags };
}

void ProcessData::updateData() {
  _curupdateusecs       = getTimeInUsecs();
  _usecssincelastupdate = _curupdateusecs - _lastupdateusecs;
  _updatedelta          = float(_usecssincelastupdate) / 1000000;
  _lastupdateusecs      = _curupdateusecs;
  for (unsigned i = 0; i < ProcessFields::END; ++i) {
    if (this->_updateFunctions[i]) {
      (*this.*_updateFunctions[i])();
    }
  }
}

void ProcessData::formatCommandLine() {
  const std::string INTERPRETERS[] = {
    "sh","bash","python","expect","perl"
    }; const unsigned S_INTERPRETERS = sizeof(INTERPRETERS) / sizeof(std::string);
  const std::string WRAPPERS[] = {
    "nohup","ssh","xfce4-terminal","mono","strace","wine","electron"
    }; const unsigned S_WRAPPERS = sizeof(WRAPPERS) / sizeof(std::string);
  static std::regex regex_commname(
    std::string("^([^\\s]*\\/)*")+    //(path to file)      0 or more non-whitespace characters followed by a slash, occuring 0 or more times
    std::string("([^\\s$]*?)")+       //(actual executable) 0 or more non-whitespace / end-of-line characters, matching as few as possible
    std::string("\\ ([^$]*?)?$")      //(flags and args)    whitespace followed by the remainder of the command line, or nothing at all
    );
  static std::regex regex_commname_ext(
    std::string("^(?:")                                 + //Flags:
    std::string(  "\\-[^\\-\\s]\\s*|")                  + //  A single letter flag that may or may not be followed by whitespace (what about - at the end)
    std::string(  "\\-\\-[^\\s]+\\s*(?:=\\s*[^\\s]+)?") + //  A multi-letter flag which may or may not have an = portion
    std::string(")*")                                   + //May have zero or more flags
    std::string("([^\\s]*\\/)*")                        + //Path to next command (as above)
    std::string("([^\\s$]*?)")                          + //Actual executable (as above)
    std::string("\\ ([^$]*?)?$")                          //Remainder of command flags and arguments
    );
  static std::string sshstart("^([^$]*)(ssh)\\s+");
  static std::string sshflags(
    std::string("((?:(?:"                                      ) + //Space between ssh / last flag set and current flag set / host name
    std::string(  "\\-[46AaCfGgKkMNnqsTtVvXxYy]+\\s*|"         ) + //A chain of one or more single letter flags without arguments
    std::string(  "\\-[bcDEeFIiJLlmOopQRSWw]\\s*[^\\s]+[\\s$]*") + //A single letter command that takes arguments
    std::string(")*\\s+)*)"                                    )   //Zero or more ssh flags
    );
  static std::string sshhostname("([^\\s\\-]+[^\\s]*)");
  static std::string sshcomm("(\\s*[^$]+?)?$");           //TODO: use as last part of commname_re ?
  // static std::regex regex_ssh(sshstart+sshflags+sshhostname+sshflags+sshcomm);
  static std::regex regex_ssh(sshflags+sshhostname+sshflags+sshcomm);

  static std::smatch sm, sshm;

  std::string com_rem = _cmdline;
  std::string com_path = "";
  std::string com_name = "";

  std::regex_search(com_rem,sm,regex_commname);
  while(true) {
    bool special = false;

    com_path = sm[1].str();
    com_name = sm[2].str();
    com_rem  = sm[3].str();

    _fcmdline.push_back({ format("%s",com_path.c_str()), A_DIM | COLOR_PAIR( col::WHT) });  //Path to command

    if (com_name.compare("ssh") == 0) {
      std::regex_search(com_rem,sshm,regex_ssh);
      if (sshm.size() > 4) {
        _fcmdline.push_back({(com_name+" ").c_str(), A_REVERSE | COLOR_PAIR(col::YLW) });
        _fcmdline.push_back({sshm[1].str().c_str(), COLOR_PAIR(col::WHT) });  //SSH flags
        _fcmdline.push_back({sshm[2].str().c_str(), COLOR_PAIR(col::MGN) });  //SSH host
        _fcmdline.push_back({sshm[3].str().c_str(), COLOR_PAIR(col::WHT) });  //More SSH flags
        std::string sshtrail = sshm[4].str();                                 //Actual SSH command
        std::regex_search(sshtrail,sm,regex_commname); continue;
      }
    }
    if (com_name.compare("sudo") == 0) {
      _fcmdline.push_back({(com_name+" ").c_str(), A_REVERSE | COLOR_PAIR(col::RED) });
      std::regex_search(com_rem,sm,regex_commname_ext); continue;
    }
    for (unsigned i = 0; i < S_INTERPRETERS; ++i) {
      if (com_name.compare(INTERPRETERS[i]) == 0) {
        _fcmdline.push_back({(com_name+" ").c_str(), COLOR_PAIR(col::GRN) });
        std::regex_search(com_rem,sm,regex_commname_ext); special = true; break;
      }
    } if (special) { continue; }
    for (unsigned i = 0; i < S_WRAPPERS; ++i) {
      if (com_name.compare(WRAPPERS[i]) == 0) {
        _fcmdline.push_back({(com_name+" ").c_str(), COLOR_PAIR(col::YLW) });
        std::regex_search(com_rem,sm,regex_commname_ext); special = true; break;
      }
    } if (special) { continue; }
    break;

  }
  _fcmdline.push_back({ format("%s",com_name.c_str()), COLOR_PAIR( col::CYN) });  //Proper base comand
  _fcmdline.push_back({ format(" %s",com_rem.c_str()), COLOR_PAIR( col::WHT) });  //Additional flags
  return;
}

void ProcessData::writeData() {
  for (unsigned i = 0; i < ProcessFields::command; ++i) {
    CursesEntry ce = (*this.*_writeFunctions[i])();
    curprintDirect(ce.flags,ce.value);
  }
  for (unsigned i = 0; i < _fcmdline.size(); ++i) {
    curprintDirect(_fcmdline[i].flags,_fcmdline[i].value);
  }
  curprintDirect("\n");
}

ProcessData::~ProcessData() {

}

}
