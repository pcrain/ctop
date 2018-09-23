#include "util.h"
#include "ptop.h"
#include "tcpdump.h"

int main(int argc, char** argv) {
  // ptop::call("sudo -K");
  ptop::call("sudo true");
  return ptop::run(argc,argv);
  // return ptop::tcpdump();
  return 0;
}
