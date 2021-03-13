#include "moonlight/ansi.h"
#include <iostream>

using namespace moonlight;

int main(int argc, char** argv) {
   (void) argc;
   (void) argv;
   std::cout << "Hello from the " << fg::bright::red("U") << fg::bright::white("S") << fg::bright::blue("A") << "!" << std::endl;
   return 0;
}
