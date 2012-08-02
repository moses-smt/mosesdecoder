#ifndef moses_MmapAllocator_h
#define moses_MmapAllocator_h

#include <limits>
#include <iostream>
#include <sys/mman.h>
#include <cstdio>
#include <unistd.h>

namespace Moses
{
    template <class T>
    class MmapAllocator
    {
      protected:    
        std::FILE* m_file_ptr;
        size_t m_file_desc;
        
        size_t m_page_size;
        size_t m_map_size;
        
        char* m_data_ptr;
        size_t m_data_offset;
        bool m_fixed;
        
      public:
        typedef T        value_type;
        typedef T*       pointer;
        typedef const T* const_pointer;
        typedef T&       reference;
        typedef const T& const_reference;
        typedef std::size_t    size_type;
        typedef std::ptrdiff_t difference_type;
    
        MmapAllocator() throw()
         : m_file_ptr(std::tmpfile()), m_file_desc(fileno(m_file_ptr)),
           m_page_size(sysconf(_SC_PAGE_SIZE)), m_map_size(0), m_data_ptr(0),
           m_data_offset(0), m_fixed(false)
        { }
        
        MmapAllocator(std::FILE* f_ptr) throw()
         : m_file_ptr(f_ptr), m_file_desc(fileno(m_file_ptr)),
           m_page_size(sysconf(_SC_PAGE_SIZE)), m_map_size(0), m_data_ptr(0),
           m_data_offset(0), m_fixed(false)
        { }
        
        MmapAllocator(std::FILE* f_ptr, size_t data_offset = 0) throw()
         : m_file_ptr(f_ptr), m_file_desc(fileno(m_file_ptr)),
           m_page_size(sysconf(_SC_PAGE_SIZE)), m_map_size(0), m_data_ptr(0),
           m_data_offset(data_offset), m_fixed(true)
        { }
        
        MmapAllocator(std::string fileName) throw()
         : m_file_ptr(std::fopen(fileName.c_str(), "wb+")), m_file_desc(fileno(m_file_ptr)),
           m_page_size(sysconf(_SC_PAGE_SIZE)), m_map_size(0), m_data_ptr(0),
           m_data_offset(0), m_fixed(false)
        { }
            
        MmapAllocator(const MmapAllocator& c) throw()
         : m_file_ptr(c.m_file_ptr), m_file_desc(c.m_file_desc),
           m_page_size(c.m_page_size), m_map_size(c.m_map_size),
           m_data_ptr(c.m_data_ptr), m_data_offset(c.m_data_offset),
           m_fixed(c.m_fixed)
        { }
        
        ~MmapAllocator() throw()
        {
            if(m_data_ptr)
            {
                munmap(m_data_ptr, m_map_size);
                if(!m_fixed && std::ftell(m_file_ptr) != -1)
                    std::fclose(m_file_ptr);
            }
        }
     
        template <class U>
        struct rebind {
            typedef MmapAllocator<U> other;
        };
     
        pointer address (reference value) const
        {
            return &value;
        }
     
        const_pointer address (const_reference value) const
        {
            return &value;
        }
     
        size_type max_size () const throw()
        {
            return std::numeric_limits<size_t>::max() / sizeof(value_type);
        }
     
        pointer allocate (size_type num, const void* = 0)
        {
            m_map_size = num * sizeof(T);
            
            if(!m_fixed)
            {
                ftruncate(m_file_desc, m_map_size);
                m_data_ptr = (char*)mmap(0, m_map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                                       m_file_desc, 0);
                return (pointer)m_data_ptr;
            }
            else
            {
                size_t map_offset = (m_data_offset / m_page_size) * m_page_size;
                size_t relative_offset = m_data_offset - map_offset;
                
                size_t map_size = m_map_size + relative_offset;
            
                m_data_ptr = (char*)mmap(0, map_size, PROT_READ, MAP_SHARED,
                                       m_file_desc, map_offset);

                return (pointer)(m_data_ptr + relative_offset);
            }
        }
     
        void deallocate (pointer p, size_type num)
        {
            if(!m_fixed)
                munmap(p, num * sizeof(T));
            else {
                size_t map_offset = (m_data_offset / m_page_size) * m_page_size;
                size_t relative_offset = m_data_offset - map_offset;
                munmap((pointer)((char*)p - relative_offset), num * sizeof(T));    
            }
            
        }
    
        void construct (pointer p, const T& value)
        {
            if(!m_fixed)
                new(p) value_type(value);
        }
        void destroy (pointer p)
        {
            if(!m_fixed)
                p->~T();
        }
         
        template <class T1, class T2>
        friend bool operator== (const MmapAllocator<T1>&, const MmapAllocator<T2>&) throw();
    
        template <class T1, class T2>
        friend bool operator!= (const MmapAllocator<T1>&, const MmapAllocator<T2>&) throw();
    };
    
    template <class T1, class T2>
    bool operator== (const MmapAllocator<T1>& a1,
                     const MmapAllocator<T2>& a2) throw()
    {    
        bool equal = true;
        equal &= a1.m_file_ptr == a2.m_file_ptr;
        equal &= a1.m_file_desc == a2.m_file_desc;
        equal &= a1.m_page_size == a2.m_page_size;
        equal &= a1.m_map_size == a2.m_map_size;
        equal &= a1.m_data_ptr == a2.m_data_ptr;
        equal &= a1.m_data_offset == a2.m_data_offset;
        equal &= a1.m_fixed == a2.m_fixed;
        return equal;
    }
    
    template <class T1, class T2>
    bool operator!=(const MmapAllocator<T1>& a1,
                     const MmapAllocator<T2>& a2) throw()
    {
        return !(a1 == a2);    
    }

}

#endif