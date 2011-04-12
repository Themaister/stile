#include "image.hpp"
#include <stdlib.h>

int main(int argc, char **argv)
{
   if (argc != 2)
      return 1;

   stile::Image a(argv[1]);
}
