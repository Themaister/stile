#include "image.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>


namespace args
{
   struct path_settings
   {
      std::string base;
      std::string palette;
      std::string tile;
      std::string tilemap;
      std::string input;
   } path;

   struct color_settings
   {
      unsigned num;

      // Designates a specific color to be treated as transparent (color #0).
      uint32_t transparent;
      // Translates the transparent color in the palette to a certain color 
      // (e.g. have green transparent, but is translated to black when creating tiles ...)
      uint32_t translate;
   } color;

   static void init_args()
   {
      path.base = "out";
      color.num = 16;
   }

   static void finish_args()
   {
      path.palette = path.base + ".pal";
      path.tile = path.base + ".tle";
      path.tilemap = path.base + ".map";
   }

   static void print_help()
   {
   }

   static void parse(int argc, char **argv)
   {
      init_args();
      
      if (argc < 2)
      {
         print_help();
         throw std::runtime_error("Not enough arguments given.");
      }

      finish_args();
   }

}

namespace output
{

   static void dump_files(const stile::Image& img)
   {
      const char *data;
      size_t size;

      std::fstream pal(args::path.palette.c_str(), std::ios::binary | std::ios::out);
      if (!pal.is_open())
         throw std::runtime_error("Failed to open output file.");
      data = (const char*)img.get_palette(size);
      pal.write(data, size * sizeof(uint16_t));

      std::fstream map(args::path.tilemap.c_str(), std::ios::binary | std::ios::out);
      if (!map.is_open())
         throw std::runtime_error("Failed to open output file.");
      data = (const char*)img.get_tilemap(size);
      map.write(data, size * sizeof(uint16_t));

      std::fstream tle(args::path.tile.c_str(), std::ios::binary | std::ios::out);
      if (!tle.is_open())
         throw std::runtime_error("Failed to open output file.");
      data = (const char*)img.get_tiles(size);
      tle.write(data, size);
   }
}

int main(int argc, char **argv)
{
   try
   {
      args::parse(argc, argv);
      stile::Image img(argv[1]);
      output::dump_files(img);
   }
   catch (std::exception& e) { fprintf(stderr, "%s\n", e.what()); }
}
