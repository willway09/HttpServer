#include "Server.hpp"
#include "unistd.h"

int main() {
  std::cout << "Here" << std::endl;

  Server server(8000);

  while(true) {
    sleep(1);
  }
}
