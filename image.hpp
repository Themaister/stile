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

         static void ARGB_to_XBGR_SSE2(uint16_t *xbgr, const uint32_t *argb, unsigned width, unsigned height);
         void generate_palette();

         std::map<uint16_t, unsigned> palette;
   };
}

#endif
