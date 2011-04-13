#include "image.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
   if (argc != 2)
      return 1;

   try
   {
      stile::Image img(argv[1]);

      const char *data;
      size_t size;

      std::fstream pal("test.pal", std::ios::binary | std::ios::out);
      if (pal.is_open())
      {
         data = (const char*)img.get_palette(size);
         pal.write(data, size * 2);
      }

      std::fstream map("test.map", std::ios::binary | std::ios::out);
      if (map.is_open())
      {
         data = (const char*)img.get_tilemap(size);
         map.write(data, size * 2);
      }

      std::fstream tle("test.tle", std::ios::binary | std::ios::out);
      if (tle.is_open())
      {
         data = (const char*)img.get_tiles(size);
         tle.write(data, size);
      }
   }
   catch (std::exception& e) { fprintf(stderr, "%s\n", e.what()); }

   
}
