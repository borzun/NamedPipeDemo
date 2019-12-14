#include <iostream>
#include "Server.h"

int main(int argc, int** argv) {
  {
    Server server{"\\\\.\\pipe\\demo_pipe"};
    if (!server.Start()) {
      std::cerr << "FATAL FAILURE - closing a program!" << std::endl;
      return -1;
    }
  }
  return 0;
}
