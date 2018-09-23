#include "tcpdump.h"

namespace ptop {

std::unordered_map<std::string,std::string> portinfo;
std::unordered_map<std::string,int> iwrite;
std::unordered_map<std::string,int> iread;

// const char* TCPDUMP_CMD = "sudo tcpdump -i any -l -s 0 -nn --immediate-mode";
const char* TCPDUMP_CMD = "sudo tcpdump -i any -l -s 0 -nn --immediate-mode 2>/dev/null";
const char* SSCOM       = "sudo ss -unpat";

void updatePortInfo() {
  std::array<char, MAX_LINE_LENGTH> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(SSCOM, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), MAX_LINE_LENGTH, pipe.get()) != nullptr) {
      std::string dstring(buffer.data());
      size_t colonpos, spacepos, equalpos, commapos, startpos = 0;
      bool success = false;
      while (true) {
        colonpos = dstring.find(":",startpos);
        if (colonpos == std::string::npos) {
          break;
        }
        char after = dstring.at(colonpos+1);
        if (after == ':' || after == ']' || after == 'P') {
          startpos = colonpos+1;
          continue;
        }
        spacepos = dstring.find(" ",colonpos);
        success = true;
        break;
      }
      if (!success) {
        continue;
      }
      equalpos = dstring.find("=",spacepos);
      if (equalpos == std::string::npos) {
        continue;
      }
      commapos = dstring.find(",",equalpos);
      if (commapos == std::string::npos) {
        continue;
      }

      std::string port = dstring.substr(colonpos+1,spacepos-colonpos-1);
      std::string pid  = dstring.substr(equalpos+1,commapos-equalpos-1);
      // std::cout << "Port: " << port << "   PID: " << pid << std::endl;
      portinfo[port] = pid;
    }
  }

  // for (std::map<std::string,std::string>::iterator portiterator = portinfo.begin();
  //   portiterator != portinfo.end();
  //   ++portiterator) {
  //     std::cout
  //       << portiterator->first
  //       << ": "
  //       << portiterator->second
  //       << ": w"
  //       << iwrite[portiterator->second]
  //       << ": r"
  //       << iread[portiterator->second]
  //       << std::endl;
  // }
}

int tcpdump() {
  long long int curtime  = getTimeInUsecs();
  long long int lasttime = curtime;
  long long int tdelta   = 0;

  updatePortInfo();

  std::array<char, MAX_LINE_LENGTH> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(TCPDUMP_CMD, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (!feof(pipe.get())) {
    if (shouldExit) {
      return 0;
    }
    if (fgets(buffer.data(), MAX_LINE_LENGTH, pipe.get()) != nullptr) {
      std::string dstring(buffer.data());
      size_t length_pos = dstring.find("length ");
      if (length_pos == std::string::npos) {
        continue;
      }
      std::string length = dstring.substr(length_pos+6);

      size_t ip_pos = dstring.find("IP ");
      if (ip_pos == std::string::npos) {
        continue;
      }

      size_t source_pos = ip_pos;
      for (unsigned i = 0; i < 4; ++i) {
        source_pos = dstring.find('.',source_pos+1);
      }
      size_t source_end_pos = dstring.find(' ',source_pos);
      std::string source_port = dstring.substr(source_pos+1,source_end_pos-source_pos-1);

      size_t dest_pos = source_end_pos;
      for (unsigned i = 0; i < 4; ++i) {
        dest_pos = dstring.find('.',dest_pos+1);
      }
      size_t dest_end_pos = dstring.find(' ',dest_pos);
      std::string dest_port = dstring.substr(dest_pos+1,dest_end_pos-dest_pos-2);

      // std::cout << "Source: " << source_port << "   Dest: " << dest_port << "   Length: " << length << std::endl;

      curtime = getTimeInUsecs();
      tdelta = curtime-lasttime;
      if (tdelta > _sleep_usecs) {
        updatePortInfo();
        lasttime = curtime;
      }
      if (portinfo.find(source_port) != portinfo.end() ) {
        iread[portinfo[source_port]] += stoi(length);
      }
      if (portinfo.find(dest_port) != portinfo.end() ) {
        iwrite[portinfo[dest_port]] += stoi(length);
      }
    }
  }

  return 0;
}

}
