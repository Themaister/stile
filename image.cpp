#include "mem.hpp"
#include "image.hpp"
#include <algorithm>
#include <stdexcept>
#include <Imlib2.h>
#include <string.h>

#include <emmintrin.h>
#include <stdio.h>

namespace stile
{
#if 0
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
#endif

   // Is this even possible with SSE? :(
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
            palette_array[palette_size & 0xf] = buf[i];
         }
      }

      if (palette.size() > 16)
         throw std::runtime_error("Palette is bigger than 16 colors!");

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

   // Super trivial hash pulled out of my ass, but hey. It's not crypto after all ;)
   void Image::generate_tilemap(const uint8_t *buf, unsigned tiles)
   {
      const uint8_t *orig_buf = buf;

      unsigned tile_index = 0;
      for (unsigned j = 0; j < tiles; j++)
      {
         uint32_t hash = 0;
         for (unsigned i = 0; i < 32; i++)
         {
            hash += ~buf[i];
            hash ^= buf[i] << 2;
         }

         if (tile_hash[hash].empty()) // Yay, a brand new tile.
         {
            fprintf(stderr, ":: Found new tile!\n");
            m_tilemap[j] = tile_index++;
            tile_hash[hash].push_back(std::pair<unsigned, unsigned>(j, m_tilemap[j]));

            // Add the new tile to our tileset.
            m_tiles.insert(m_tiles.end(), buf, buf + 32);
         }
         else // Hmm... Hopefully we can find a tile that was already used.
         {
            int tilenum = -1;
            for (std::list<std::pair<unsigned, unsigned> >::const_iterator itr = tile_hash[hash].begin();
                  itr != tile_hash[hash].end(); ++itr)
            {
               if (memcmp(buf, &orig_buf[itr->first * 32], 32) == 0) // Yay :)
               {
                  tilenum = itr->second;
                  break;
               }
            }

            // We found an old tile, use it! :D
            if (tilenum >= 0)
            {
               m_tilemap[j] = tilenum;
            }
            else // Oh snap... Chain our hash table even more :(
            {
               m_tilemap[j] = tile_index++;
               tile_hash[hash].push_back(std::pair<unsigned, unsigned>(j, m_tilemap[j]));

               fprintf(stderr, ":: Found new tile!\n");
               m_tiles.insert(m_tiles.end(), buf, buf + 32);
            }
         }

         buf += 32;
      }
   }


   Image::Image(const char *path, unsigned palette_size)
   {
      std::fill(palette_array, palette_array + 16, 0);

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
      m_tilemap.reserve(width * height / (8 * 8));

      const uint32_t *data = imlib_image_get_data_for_reading_only();
      std::copy(data, data + width * height, picture_buf.begin());
      imlib_free_image();

      ARGB_to_XBGR_SSE2(&m_buf[0], &picture_buf[0], width, height);

      generate_palette(&m_buf[0], width, height);
      convert_to_bitplane(&m_bitbuf[0], &m_buf[0], width, height);

      tiles_x = width / 8;
      tiles_y = height / 8;
      generate_tilemap(&m_bitbuf[0], tiles_x * tiles_y);

      puts("BG stats:");
      printf("\tSize: %u x %u\n", width, height);
      printf("\tDifferent tiles: %u\n", (unsigned)m_tiles.size() / 32);
      printf("\tColors: %u\n", (unsigned)palette.size());
   }

   const uint16_t* Image::get_palette(size_t& size) const
   {
      size = 16;
      return palette_array;
   }

   const uint16_t* Image::get_tilemap(size_t& size) const
   {
      size = tiles_x * tiles_y;
      return &m_tilemap[0];
   }

   const uint8_t* Image::get_tiles(size_t& size) const
   {
      size = m_tiles.size();
      return &m_tiles[0];
   }
}


