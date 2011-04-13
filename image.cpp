#include "mem.hpp"
#include "image.hpp"
#include <algorithm>
#include <stdexcept>
#include <Imlib2.h>

#include <emmintrin.h>
#include <stdio.h>

namespace stile
{
   static void print_vector_u32(__m128i reg)
   {
      union
      {
         __m128i vec;
         uint32_t elem[4];
      } u;

      u.vec = reg;

      printf("Vector:\n");
      for (unsigned i = 0; i < 4; i++)
      {
         printf("\tElem %u: 0x%08x\n", i, (unsigned)u.elem[i]);
      }
   }

   static void print_vector_u16(__m128i reg)
   {
      union
      {
         __m128i vec;
         uint16_t elem[4];
      } u;

      u.vec = reg;

      printf("Vector:\n");
      for (unsigned i = 0; i < 8; i++)
      {
         printf("\tElem %u: 0x%04x\n", i, (unsigned)u.elem[i]);
      }
   }

   void Image::generate_palette(const uint16_t *buf, unsigned width, unsigned height)
   {
      unsigned elems = width * height;
      for (unsigned i = 0; i < elems; i++)
      {
         std::map<uint16_t, unsigned>::const_iterator itr = palette.find(buf[i]);
         if (itr == palette.end())
         {
            unsigned palette_size = palette.size();
            palette[buf[i]] = palette_size;
         }
      }

      if (palette.size() > 16)
         fprintf(stderr, "Palette is bigger than 16 colors! %u\n", (unsigned)palette.size());

      for (std::map<uint16_t, unsigned>::const_iterator itr = palette.begin();
            itr != palette.end(); ++itr)
         printf("Color %u = 0x%04x\n", itr->second, (unsigned)itr->first);
   }

   void Image::convert_16bpp_to_4bpp(uint8_t *bitplane, const uint16_t *buf, unsigned linesize)
   {
      std::fill(bitplane, bitplane + 32, 0);
      for (unsigned y = 0; y < 8; y++)
      {
         unsigned index = 2 * y;
         for (unsigned x = 0; x < 8; x++)
         {
            unsigned shift_amt = 7 - x;
            uint16_t color = palette[buf[y * linesize + x]];
            bitplane[index +  0] |= ((color & 0x1) >> 0) << shift_amt;
            bitplane[index +  1] |= ((color & 0x2) >> 1) << shift_amt;
            bitplane[index + 16] |= ((color & 0x4) >> 2) << shift_amt;
            bitplane[index + 17] |= ((color & 0x8) >> 3) << shift_amt;
         }
      }
   }

   void Image::convert_to_bitplane(uint8_t *bitplane, const uint16_t *buf, unsigned width, unsigned height)
   {
      unsigned tiles_x = width / 8;
      unsigned tiles_y = height / 8;

      printf("Number of tiles: %u x %u\n", tiles_x, tiles_y);

      for (unsigned y = 0; y < tiles_y; y++)
      {
         for (unsigned x = 0; x < tiles_x; x++)
         {
            convert_16bpp_to_4bpp(
                  bitplane + 32 * (y * tiles_x + x), 
                  buf + y * 8 * width + x * 8,
                  width);
         }
      }

      puts("Bitplane:");
      for (unsigned i = 0; i < width * height / 2; i++)
      {
         printf("\tByte %u: 0x%02x\n", i, (unsigned)bitplane[i]);
      }
   }

   // For lulz ;)
   void Image::ARGB_to_XBGR_SSE2(uint16_t *xbgr, const uint32_t *argb, unsigned width, unsigned height)
   {
      unsigned sse_elems = width * height / 4;

      // Mask.
      __m128i mask = _mm_set1_epi32((0xF8) | (0xF8 << 8) | (0xF8 << 16));
      __m128i g_mask = _mm_set1_epi32(0x0000FF00);
      __m128i final_mask = _mm_set1_epi32(0x7FFF0000);

      // Convert 8 pixels on the go. :)
      for (unsigned i = 0; i < sse_elems; i += 4)
      {
         __m128i val[4];
         __m128i g[4];
         __m128i final_res[4];
         __m128i res[2];

         val[0] = _mm_load_si128((const __m128i*)argb + i + 0);
         val[1] = _mm_load_si128((const __m128i*)argb + i + 1);
         val[2] = _mm_load_si128((const __m128i*)argb + i + 2);
         val[3] = _mm_load_si128((const __m128i*)argb + i + 3);

         // Mask to get 5 bits from each channel.
         val[0] = _mm_and_si128(val[0], mask);
         val[1] = _mm_and_si128(val[1], mask);
         val[2] = _mm_and_si128(val[2], mask);
         val[3] = _mm_and_si128(val[3], mask);

         // Move R and B channel where it belongs.
         val[0] = _mm_srli_epi32(val[0], 3);
         val[1] = _mm_srli_epi32(val[1], 3);
         val[2] = _mm_srli_epi32(val[2], 3);
         val[3] = _mm_srli_epi32(val[3], 3);

         val[0] = _mm_or_si128(val[0], _mm_slli_epi32(val[0], 26));
         val[1] = _mm_or_si128(val[1], _mm_slli_epi32(val[1], 26));
         val[2] = _mm_or_si128(val[2], _mm_slli_epi32(val[2], 26));
         val[3] = _mm_or_si128(val[3], _mm_slli_epi32(val[3], 26));

         // Masks to remove the B and R.
         g[0] = _mm_and_si128(val[0], g_mask);
         g[1] = _mm_and_si128(val[1], g_mask);
         g[2] = _mm_and_si128(val[2], g_mask);
         g[3] = _mm_and_si128(val[3], g_mask);

         // Shift G channel in place.
         val[0] = _mm_or_si128(val[0], _mm_slli_epi32(g[0], 13));
         val[1] = _mm_or_si128(val[1], _mm_slli_epi32(g[1], 13));
         val[2] = _mm_or_si128(val[2], _mm_slli_epi32(g[2], 13));
         val[3] = _mm_or_si128(val[3], _mm_slli_epi32(g[3], 13));

         val[0] = _mm_and_si128(val[0], final_mask);
         val[1] = _mm_and_si128(val[1], final_mask);
         val[2] = _mm_and_si128(val[2], final_mask);
         val[3] = _mm_and_si128(val[3], final_mask);

         // Stuff is now interleaved in memory. Shuffle it back into place.
         // val =  [c3, 0, c2, 0, c1, 0, c0, 0];
         // val2 = [c7, 0, c6, 0, c5, 0, c4, 0];
         //
         // After hi-shuffle:
         // val = [c3, c2, 0, 0, c1, 0, c0, 0];
         // Lo-shuffle:
         // val = [c3, c2, 0, 0, c1, c0, 0, 0];
         //
         // Hi-shuffle and lo-shuffle on val2:
         // val2 = [0, 0, c7, c6, 0, 0, c5, c4];
         //
         // OR these together:
         // res = [c3, c2, c7, c6, c1, c0, c5, c4];
         //
         // Do final shuffle:
         // res = [c7, c6, c5, c4, c3, c2, c1, c0];
         //
         // And write back, the writeback still follows a "little-endian" order, so c0 through c7 is written.
         final_res[0] = _mm_shufflehi_epi16(val[0], _MM_SHUFFLE(3, 1, 0, 0));
         final_res[1] = _mm_shufflehi_epi16(val[1], _MM_SHUFFLE(0, 0, 3, 1));
         final_res[2] = _mm_shufflehi_epi16(val[2], _MM_SHUFFLE(3, 1, 0, 0));
         final_res[3] = _mm_shufflehi_epi16(val[3], _MM_SHUFFLE(0, 0, 3, 1));
         final_res[0] = _mm_shufflelo_epi16(final_res[0], _MM_SHUFFLE(3, 1, 0, 0));
         final_res[1] = _mm_shufflelo_epi16(final_res[1], _MM_SHUFFLE(0, 0, 3, 1));
         final_res[2] = _mm_shufflelo_epi16(final_res[2], _MM_SHUFFLE(3, 1, 0, 0));
         final_res[3] = _mm_shufflelo_epi16(final_res[3], _MM_SHUFFLE(0, 0, 3, 1));

         res[0] = _mm_or_si128(final_res[0], final_res[1]);
         res[1] = _mm_or_si128(final_res[2], final_res[3]);
         res[0] = _mm_shuffle_epi32(res[0], _MM_SHUFFLE(2, 0, 3, 1));
         res[1] = _mm_shuffle_epi32(res[1], _MM_SHUFFLE(2, 0, 3, 1));

         _mm_store_si128((__m128i*)xbgr + (i >> 1), res[0]);
         _mm_store_si128((__m128i*)xbgr + (i >> 1) + 1, res[1]);
      }
   }


   Image::Image(const char *path, unsigned palette_size)
   {
      Imlib_Image img;

      img = imlib_load_image(path);
      if (!img)
         throw std::runtime_error("Failed to open file ...");

      imlib_context_set_image(img);

      unsigned width = imlib_image_get_width();
      unsigned height = imlib_image_get_height();
      mem::aligned_vector<uint32_t, 16> picture_buf;
      picture_buf.reserve(width * height);
      m_buf.reserve(width * height);
      m_bitbuf.reserve(width * height / 2);

      const uint32_t *data = imlib_image_get_data_for_reading_only();
      std::copy(data, data + width * height, picture_buf.begin());
      imlib_free_image();

      ARGB_to_XBGR_SSE2(&m_buf[0], &picture_buf[0], width, height);

      for (unsigned i = 0; i < width * height; i++)
      {
         printf("Input pixel  %5u: 0x%08x\n", i, (unsigned)picture_buf[i]);
         printf("Output Pixel %5u: 0x%04x\n", i, (unsigned)m_buf[i]);
      }

      generate_palette(&m_buf[0], width, height);
      convert_to_bitplane(&m_bitbuf[0], &m_buf[0], width, height);
   }
}


