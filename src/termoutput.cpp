#include "termoutput.h"

namespace ptop {

int _start_x = 0;
int _start_y = 0;
int _term_h = 0;
int _term_w = 0;

void init_curses() {
  initscr();                // Initialize the screen
  start_color();            // Use colors
  // attron(A_BOLD);           // All text is bold
  use_default_colors();     // Use default colors
  curs_set(0);              // Make cursor invisible
  for (int i = 0; i <= 8; ++ i)
    init_pair(i,i%8,-1);


  getmaxyx(stdscr, _term_h, _term_w);

  pad = newpad(MAX_HEIGHT,MAX_WIDTH);
  keypad(pad, TRUE); //Enable the keypad (arrow keys count as single keys)

  inputThread = new std::thread(inputLoop);
}

void inputLoop() {
  while (!shouldExit) {
    int input = wgetch(pad);
    switch(input) {
      case 'q':
        shouldExit = true;
        return;
      case 'a': ProcessData::setProcessSortMode(ProcessFields::age);      break;
      case 'A': ProcessData::setProcessSortMode(ProcessFields::cpuavg);   break;
      case 'p': ProcessData::setProcessSortMode(ProcessFields::cpu);      break;
      case 'P': ProcessData::setProcessSortMode(ProcessFields::pid);      break;
      case 't': ProcessData::setProcessSortMode(ProcessFields::cputime);  break;
      case 'c': ProcessData::setProcessSortMode(ProcessFields::command);  break;
      case 'U': ProcessData::setProcessSortMode(ProcessFields::user);     break;
      case 'r': ProcessData::setProcessSortMode(ProcessFields::read);     break;
      case 'w': ProcessData::setProcessSortMode(ProcessFields::write);    break;
      case 'u': ProcessData::setProcessSortMode(ProcessFields::upload);   break;
      case 'd': ProcessData::setProcessSortMode(ProcessFields::download); break;
      case 'n': ProcessData::setProcessSortMode(ProcessFields::priority); break;
      case 's': ProcessData::setProcessSortMode(ProcessFields::status);   break;
      case 'm': ProcessData::setProcessSortMode(ProcessFields::ram);      break;
      case 'y': ProcessData::setProcessSortMode(ProcessFields::tty);      break;
      case 'i': ProcessData::_showAverageUsage ^= true;                   break;
      case KEY_UP:
        _start_y -= SCROLL_SPEED;
        if (_start_y < 0) {
          _start_y = 0;
        }
        break;
      case KEY_DOWN:
        _start_y += SCROLL_SPEED;
        break;
      case KEY_LEFT:
        _start_x -= SCROLL_SPEED;
        if (_start_x < 0) {
          _start_x = 0;
        }
        break;
      case KEY_RIGHT:
        _start_x += SCROLL_SPEED;
        break;
    }
    // usleep(100000);
  }
}

void refresh_pad() {
  // wrefresh(pad);
  prefresh(pad, 0, _start_x, 0, 0, 1, _term_w-1);
  prefresh(pad, _start_y+1, _start_x, 1, 0, _term_h-2, _term_w-1);
  // _start_y += 1;
  // pnoutrefresh(pad, 0, 0, 0, 0, _term_h, _term_w);
}

void end_curses() {
  inputThread->join();
  delete inputThread;
  curs_set(1);              // Make cursor visible again
  endwin();
}

void curprint(int flags, const char* format, ...) {
  wattrset(pad,flags);
  va_list argptr;
  va_start(argptr, format);
  std::string str = vformat(format, argptr);
  waddstr(pad,str.c_str());
  va_end(argptr);
}

void curprint(const char* format, ...) {
  wattrset(pad,COLOR_PAIR(col::WHT));
  va_list argptr;
  va_start(argptr, format);
  std::string str = vformat(format, argptr);
  waddstr(pad,str.c_str());
  va_end(argptr);
}

void curprintDirect(int flags, std::string str) {
  wattrset(pad,flags);
  waddstr(pad,str.c_str());
}

void curprintDirect(std::string str) {
  wattrset(pad,COLOR_PAIR(col::WHT));
  waddstr(pad,str.c_str());
}

void curreset() {
  wmove(pad,0,0);
}

void curset(int ypos, int xpos) {
  wmove(pad,ypos,xpos);
}

void curclear() {
  refresh_pad();
  // wrefresh(pad);
  wclear(pad);
}

void erasePad() {
  werase(pad);
}

void writeEmptyLine() {
  wclrtoeol(pad);
  waddstr(pad,"\n");
}

void moveCursorToBeginningOfScreen() {
  wmove(pad,_start_y+1,0);
}

}
