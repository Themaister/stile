#include "image.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>
#include <getopt.h>


namespace args
{
   struct path_settings
   {
      std::string base;
      std::string palette;
      std::string tile;
      std::string tilemap;
      std::string input;
   };
   static path_settings path;

   struct color_settings
   {
      unsigned num;

      // Designates a specific color to be treated as transparent (color #0).
      uint32_t transparent;
      // Translates the transparent color in the palette to a certain color 
      // (e.g. have green transparent, but is translated to black when creating tiles ...)
      uint32_t translate;
   };
   static color_settings color;

   static unsigned size;
   static bool verbose;

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
      std::cout << "=====================================" << std::endl;
      std::cout << " stile - Simple SNES tile exporter" << std::endl;
      std::cout << "=====================================" << std::endl;
      std::cout << std::endl;
      std::cout << "\tUsage: stile [OPTIONS...]" << std::endl;
      std::cout << std::endl;
      std::cout << "  -f/--file: Input file." << std::endl;
      std::cout << "  -o/--output: Output base path (extensions will be appended)." << std::endl;
      std::cout << "  -c/--colors: Number of colors to use (default: 16)." << std::endl;
      std::cout << "  -S/--sprite: Sprite mode. Do not export tile map." << std::endl;
      std::cout << "  -s/--size: Override tile/sprite size (default: 8)." << std::endl;
      std::cout << "  -t/--transparent: Pick transparent color in XBGR hex format." << std::endl;
      std::cout << "  -T/--translate: Translates transparent color." << std::endl;
      std::cout << "  -h/--help: This help." << std::endl;
      std::cout << "  -v/--verbose: Verbose logging." << std::endl;
      std::cout << std::endl;
   }

   static void parse(int argc, char **argv)
   {
      init_args();
      
      if (argc < 2)
      {
         print_help();
         throw std::runtime_error("Not enough arguments given.");
      }

      int val = 0;
      struct option opts[] = {
         { "help", 0, NULL, 'h' },
         { "file", 1, NULL, 'f' },
         { "output", 1, NULL, 'o' },
         { "colors", 1, NULL, 'c' },
         { "sprite", 0, NULL, 'S' },
         { "size", 1, NULL, 's' },
         { "transparent", 1, NULL, 't' },
         { "translate", 1, NULL, 'T' },
         { "verbose", 0, NULL, 'v' }
      };

      int option_index = 0;
      char optstring[] = "hf:o:c:Ss:vt:T:";
      for (;;)
      {
         val = 0;
         int c = getopt_long(argc, argv, optstring, opts, &option_index);

         if (c == -1)
            break;

         switch (c)
         {
            case 'h':
               print_help();
               exit(0);

            case 'f':
               args::path.input = optarg;
               break;

            case 'o':
               args::path.base = optarg;
               break;

            case 'c':
               args::color.num = strtoul(optarg, NULL, 0);
               break;

            case 's':
               args::size = strtoul(optarg, NULL, 0);
               break;

            case 't':
               args::color.transparent = strtoul(optarg, NULL, 16);
               break;

            case 'T':
               args::color.translate = strtoul(optarg, NULL, 16);
               break;

            case 'v':
               args::verbose = true;
               break;
         }

         if (optind != argc)
         {
            print_help();
            throw std::runtime_error("Invalid arguments.");
         }

         if (args::path.input.empty())
         {
            print_help();
            throw std::runtime_error("Needs an input file!");
         }
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
   catch (std::exception& e) { std::cerr << "ERROR: " << e.what() << std::endl; return 1; }
}
