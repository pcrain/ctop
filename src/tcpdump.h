#ifndef TCPDUMP_H_
#define TCPDUMP_H_

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <regex>
#include <unordered_map>

#include "util.h"
#include "processdata.h"

namespace ptop {

const unsigned MAX_LINE_LENGTH = 255;

extern unsigned _sleep_usecs;
extern bool shouldExit;
extern std::map<int,std::string> userIdMap;
extern int uptime;
extern std::unordered_map<std::string,std::string> portinfo;
extern std::unordered_map<std::string,int> iwrite;
extern std::unordered_map<std::string,int> iread;

int tcpdump();

}

#endif /* TCPDUMP_H_ */
