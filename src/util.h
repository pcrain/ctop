#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <curses.h>
#include <string>
#include <streambuf>
#include <sstream>
#include <algorithm> //std::replace

#include <cstdarg>
#include <vector>
#include <string>
#include <string.h>

#include "termoutput.h"

namespace ptop {

const unsigned char CHUNKSIZE = 4;

int signof(const float x);
// float activation(const float x);
std::string format (const char *fmt, ...);
std::string vformat (const char *fmt, va_list ap);
void fsleep(int frames);
unsigned long hexint(std::string hexstring);
float hexfloat(unsigned long hexint);
void endianfix(char (&array)[4]);
bool file_available(const char* fname);

std::string readAllLines(std::string fname);
std::string readAllLines(std::string fname, char nullReplacement);
bool readProcLines(const char* fname, char** buffer, long* filelen);

//Find the fieldNum'th occurrence of a substring in a string
std::vector<std::string> splitAndFind(std::string s, std::string delimiter);
std::string splitAndFind(std::string s, std::string delimiter, unsigned index);
// void splitAndFind(std::string s, std::string delimiter, std::vector<std::string>& buffer);
void splitAndFind(std::string s, std::string delimiter, std::string buffer[], unsigned& length);

inline long long int getTimeInUsecs() {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void call(const char* cmd);

}

#endif /* UTIL_H_ */
