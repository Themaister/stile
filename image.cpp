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

   static void test_vector()
   {
      __m128i test = _mm_set_epi32(3, 2, 1, 0);
      __m128i test_shuf = _mm_shufflehi_epi16(test, _MM_SHUFFLE(0, 1, 2, 3));
      print_vector_u32(test);
      print_vector_u32(test_shuf);
   }


   void Image::ARGB_to_XBGR_SSE2(uint16_t *xbgr, const uint32_t *argb, unsigned width, unsigned height)
   {
      test_vector();

      if (((width * height) % 8) != 0)
         throw std::runtime_error("Image does not have aligned size.");

      unsigned sse_elems = width * height / 4;

      // Mask.
      __m128i mask = _mm_set1_epi32((0xF8) | (0xF8 << 8) | (0xF8 << 16));
      __m128i g_mask = _mm_set1_epi32(0x0000FF00);
      __m128i final_mask = _mm_set1_epi32(0x7FFF0000);

      // Convert 8 pixels on the go. :)
      for (unsigned i = 0; i < sse_elems; i += 2)
      {
         // Convert first 4 pixels.
         __m128i val = _mm_load_si128((const __m128i*)argb + i);
         // Mask to get 5 bits from each channel.
         val = _mm_and_si128(val, mask);

         // Move R and B channel where it belongs.
         val = _mm_srli_epi32(val, 3);
         val = _mm_or_si128(val, _mm_slli_epi32(val, 26));

         // Masks to remove the B and R.
         __m128i g = _mm_and_si128(val, g_mask);

         // Shift G channel in place.
         val = _mm_or_si128(val, _mm_slli_epi32(g, 13));
         val = _mm_and_si128(val, final_mask);

         // Convert last 4 pixels.
         __m128i val2 = _mm_load_si128((const __m128i*)argb + i + 1);
         val2 = _mm_and_si128(val2, mask);
         val2 = _mm_srli_epi32(val2, 3);
         val2 = _mm_or_si128(val2, _mm_slli_epi32(val2, 26));
         g = _mm_and_si128(val2, g_mask);
         val2 = _mm_or_si128(val2, _mm_slli_epi32(g, 13));
         val2 = _mm_and_si128(val2, final_mask);

         // Interleave in a 0-4-1-5-2-6-3-7 pattern. Time to shuffle it into place!
         __m128i final_res[2];
         final_res[0] = _mm_shufflehi_epi16(val, _MM_SHUFFLE(3, 1, 0, 0));
         final_res[0] = _mm_shufflelo_epi16(final_res[0], _MM_SHUFFLE(3, 1, 0, 0));
         final_res[1] = _mm_shufflehi_epi16(val2, _MM_SHUFFLE(0, 0, 3, 1));
         final_res[1] = _mm_shufflelo_epi16(final_res[1], _MM_SHUFFLE(0, 0, 3, 1));
         __m128i res = _mm_or_si128(final_res[0], final_res[1]);
         res = _mm_shuffle_epi32(res, _MM_SHUFFLE(2, 0, 3, 1));

         puts("RESULT");
         print_vector_u32(res);

         _mm_store_si128((__m128i*)xbgr + (i >> 1), res);
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

      const uint32_t *data = imlib_image_get_data_for_reading_only();
      std::copy(data, data + width * height, picture_buf.begin());
      imlib_free_image();

      ARGB_to_XBGR_SSE2(&m_buf[0], &picture_buf[0], width, height);

      for (unsigned i = 0; i < width * height; i++)
      {
         printf("Input pixel  %5u: 0x%08x\n", i, (unsigned)picture_buf[i]);
         printf("Output Pixel %5u: 0x%04x\n", i, (unsigned)m_buf[i]);
      }
   }

   void Image::generate_palette()
   {
   }

}


