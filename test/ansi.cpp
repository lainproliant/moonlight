#include "moonlight/ansi.h"
#include <iostream>

using namespace moonlight;

int main(int argc, char** argv) {
   (void) argc;
   (void) argv;
   const auto red = fg::bright(fg::red);
   const auto white = fg::bright(fg::white);
   const auto blue = fg::bright(fg::blue);
   std::cout << "Hello from the " << red("U") << white("S") << blue("A") << "!" << std::endl;
   std::cout << "This text is " << fg::reverse("reversed") << "." << std::endl;
   std::cout << "This text might " << fg::blink("blink") << " if your terminal supports it." << std::endl;
   return 0;
}
