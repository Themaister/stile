#ifndef __STILE_MEM_HPP
#define __STILE_MEM_HPP

#include <stdlib.h>
#include <stddef.h>
#include <vector>
#include <memory>
#include <exception>
#include <limits>

namespace stile
{
   namespace mem
   {
      template <class T, size_t align>
      class aligned_allocator : std::allocator<T>
      {
         public:
            typedef typename std::allocator<T> base;
            typedef typename base::pointer pointer;
            typedef typename base::const_pointer const_pointer;
            typedef typename base::size_type size_type;
            typedef typename base::value_type value_type;
            typedef typename base::reference reference;
            typedef typename base::const_reference const_reference;
            typedef typename base::difference_type difference_type;

            // What is this?
            template <class U>
            struct rebind { typedef aligned_allocator<U, align> other; };

            pointer allocate(size_type n)
            {
               pointer p = NULL;
               if (posix_memalign((void**)&p, align, n * sizeof(T)) < 0)
               {
                  if (p)
                     free(p);
                  throw std::bad_alloc();
               }

               return p;
            }

            pointer allocate(size_type n, void const*)
            {
               return this->allocate(n);
            }

            void deallocate(pointer p, size_type)
            {
               free(p);
            }

            void destroy(pointer p)
            {
               base::destroy(p);
            }

            void construct(pointer p, const_reference t)
            {
               base::construct(p, t);
            }

            pointer address(reference x) const
            {
               return &x;
            }

            const_pointer address(const_reference x) const
            {
               return &x;
            }

            size_type max_size() const throw()
            {
               return std::numeric_limits<size_type>::max() / sizeof(T);
            }

         private:
      };

      template <class T, size_t align>
      class aligned_vector : public std::vector<T, aligned_allocator<T, align> >
      {};
   }
}

#endif
