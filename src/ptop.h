#ifndef PTOP_H_
#define PTOP_H_

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <dirent.h>

#include "termoutput.h"
#include "util.h"
#include "processdata.h"
#include "tcpdump.h"

namespace ptop {

const unsigned MAX_PID = 32768;
// const unsigned SLEEP_USECS = 100000;
// const unsigned SLEEP_USECS = 250000;
// const unsigned SLEEP_USECS = 1000000;

// extern unsigned long p1_state_address;

extern unsigned _sleep_usecs;
extern bool shouldExit;
extern std::map<int,std::string> userIdMap;
extern int uptime;
extern std::unordered_map<std::string,std::string> portinfo;
extern std::unordered_map<std::string,int> iwrite;
extern std::unordered_map<std::string,int> iread;
extern int _term_h, _term_w, _start_x, _start_y;

int run(int argc, char** argv);
int eventLoop();
void exit_handler(int s);
void init_exit_handler();
void exit_normal();

void writeHeader();
void generateUserIdMap();
std::string lookupUser(int uid);

}

#endif /* PTOP_H_ */
