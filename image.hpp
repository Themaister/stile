#ifndef __STILE_IMAGE_HPP
#define __STILE_IMAGE_HPP

#include "mem.hpp"
#include <stdint.h>
#include <map>

namespace stile
{
   class Image
   {
      public:
         Image(const char *path, unsigned palette_size = 16);

         const uint16_t* operator()() const;
         uint16_t operator()(unsigned x, unsigned y) const;

      private:
         mem::aligned_vector<uint16_t, 16> m_buf;
         mem::aligned_vector<uint8_t, 16> m_bitbuf;

         static void ARGB_to_XBGR_SSE2(uint16_t *xbgr, const uint32_t *argb, unsigned width, unsigned height);
         static void calculate_hash_SSE2(const uint16_t *xbgr, 
               unsigned tile_x, unsigned tile_y,
               unsigned tile_size,
               unsigned width, unsigned height);

         void generate_palette(const uint16_t *buf, unsigned width, unsigned height);
         void convert_to_bitplane(uint8_t *bitplane, const uint16_t *buf, unsigned width,
               unsigned height);
         void convert_16bpp_to_4bpp(uint8_t *bitplane, const uint16_t *buf, unsigned linesize);


         std::map<uint16_t, unsigned> palette;
   };
}

#endif
