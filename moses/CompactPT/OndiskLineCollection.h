// $Id$                                                                                                                               
// vim:tabstop=2                                                                                                                      
/***********************************************************************                                                              
Moses - factored phrase-based language decoder                                                                                        
Copyright (C) 2006 University of Edinburgh                                                                                            
                                                                                                                                      
This library is free software; you can redistribute it and/or                                                                         
modify it under the terms of the GNU Lesser General Public                                                                            
License as published by the Free Software Foundation; either                                                                          
version 2.1 of the License, or (at your option) any later version.                                                                    
                                                                                                                                      
This library is distributed in the hope that it will be useful,                                                                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                                                                        
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                                                                     
Lesser General Public License for more details.                                                                                       
                                                                                                                                      
You should have received a copy of the GNU Lesser General Public                                                                      
License along with this library; if not, write to the Free Software                                                                   
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA                                                        
***********************************************************************/  

#ifndef moses_OndiskLineCollection_h
#define moses_OndiskLineCollection_h

#include <vector>
#include <string>
#include <cstdio>

#include "ThrowingFwrite.h"
#include "MonotonicVector.h"

namespace Moses
{

class OndiskLineCollection
{
  private:
    MonotonicVector<uint64_t, uint32_t> m_positions;  
    std::FILE* m_filePtr;
    uint64_t m_currPos;
    uint64_t m_totalSize;
    
  public:
    OndiskLineCollection()
    : m_filePtr(std::tmpfile()), m_currPos(0), m_totalSize(0)
    { }
    
    void Rewind()
    {
      m_currPos = 0;
      std::rewind(m_filePtr);
      m_positions.commit();
    }
    
    bool HasNext()
    {
      return m_currPos < m_positions.size();
    }
    
    std::string GetNext()
    {
      std::string s; 
      if(m_currPos < m_positions.size())
      {
        size_t size;
        if(m_currPos + 1 < m_positions.size())
          size = m_positions[m_currPos + 1] - m_positions[m_currPos];
        else
          size = m_totalSize - m_positions[m_currPos];
          
        char buffer[size];
        std::fread(buffer, sizeof(char), size, m_filePtr);
        s = std::string(buffer, size);
        m_currPos++;
      }
      return s;
    }
    
    void PushBack(std::string s)
    {
      uint64_t pos = ftello(m_filePtr);
      m_positions.push_back(pos);
      size_t length = s.size();
      ThrowingFwrite(s.c_str(), sizeof(char), length, m_filePtr);
      m_currPos++;
      m_totalSize += s.size();
    }
    
    uint64_t Size()
    {
      return m_positions.size();
    }
    
    uint64_t Save(std::FILE* out)
    {
      uint64_t byteSize = 0;
      bool sorted = false;
      byteSize += ThrowingFwrite(&sorted, sizeof(bool), 1, out) * sizeof(bool);
      
      uint64_t test = m_positions.save(out);
      byteSize += test;
    
      byteSize += ThrowingFwrite(&m_totalSize, sizeof(uint64_t), 1, out) * sizeof(uint64_t);
      
      Rewind();
      
      size_t bufferSize = 1000000;
      std::vector<std::string> buffer;
      
      while(HasNext()) {
        std::string s = GetNext();
        buffer.push_back(s);
        
        if(buffer.size() == bufferSize) {
          for(std::vector<std::string>::iterator it = buffer.begin();
              it != buffer.end(); it++)
            byteSize += ThrowingFwrite(it->c_str(), sizeof(char), it->size(), out)
                        * sizeof(char);
          buffer.clear();
        }
      }
      
      for(std::vector<std::string>::iterator it = buffer.begin();
          it != buffer.end(); it++)
        byteSize += ThrowingFwrite(it->c_str(), sizeof(char), it->size(), out)
                    * sizeof(char);
      return byteSize;
    }
};

}

#endif
