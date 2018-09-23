#ifndef TERMOUTPUT_H_
#define TERMOUTPUT_H_

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <curses.h>
#include <thread>

#include <cstdarg>
#include <vector>
#include <string>

#include "ptop.h" //shouldExit
#include "util.h"

namespace ptop {

const int SCROLL_SPEED = 10;
const int MAX_WIDTH    = 1000;
const int MAX_HEIGHT   = 1000;

extern unsigned _sleep_usecs;
extern int _term_h, _term_w, _start_x, _start_y;
static std::thread* inputThread;
static WINDOW* pad;

extern bool shouldExit;

enum col {
  BEG,RED,GRN,YLW,BLU,MGN,CYN,WHT,BLK
};

void init_curses();
void end_curses();
void refresh_pad();
void curprint(const char* format, ...);
void curprint(int flags, const char* format, ...);
void curprintDirect(std::string str);
void curprintDirect(int flags, std::string str);
void curset(int ypos, int xpos);
void curreset();
void curclear();
void erasePad();
void writeEmptyLine();

void moveCursorToBeginningOfScreen();

void inputLoop();

}

#endif /* TERMOUTPUT_H_ */
