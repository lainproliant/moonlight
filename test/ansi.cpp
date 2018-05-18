#include "moonlight/ansi.h"
#include <iostream>

int main(int argc, char** argv) {
   (void) argc;
   (void) argv;
   auto library = moonlight::ansi::Library::create();
   auto decorate = library["bright white bg-red"];
   auto red = library["bright white bg-red"];
   auto white = library["black bg-white"];
   auto blue = library["bright white bg-blue"];
   std::cout << "Hello from the " << red("U") << white("S") << blue("A") << "!" << std::endl;
   return 0;
}
