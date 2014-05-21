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

#ifndef moses_PhrasePiece_h
#define moses_PhrasePiece_h

#include "moses/Phrase.h"
#include <boost/foreach.hpp>

namespace Moses
{
    
class PhrasePiece {
public:
    PhrasePiece(const char* input)
    : m_phraseString(input), m_mosesPhrase(0), m_hierarchical(false)
    {
        Init();
    }
    
    PhrasePiece(const std::string& input)
    : m_phraseString(input), m_mosesPhrase(0), m_hierarchical(false)
    {
        Init();
    }
    
    PhrasePiece(const Phrase& mosesPhrase, const std::vector<FactorType> &input)
    : m_phraseString(mosesPhrase.GetStringRep(input)),
      m_mosesPhrase(&mosesPhrase), m_hierarchical(false)
    {
        Init();
    }
    
    const std::string& GetString() const {
        return m_phraseString;    
    }
    
    const StringPiece& GetWord(size_t i) const {
        return m_phraseTokens[i]; 
    }
    
    size_t GetSize() const
    {
        // Do not count LHS for phrase length if hierarchical
        return m_phraseTokens.size() - (size_t)m_hierarchical;
    }
    
    const PhrasePiece GetSubPhrase(size_t start, size_t end) const {
        std::stringstream subphrase;
        
        if(start >= 0 && start <= end && end < GetSize()) {
          subphrase << m_phraseTokens[start];
          for(size_t i = start + 1; i <= end; ++i)
            subphrase << " " << m_phraseTokens[i];
                
          if(m_hierarchical)
            subphrase << " [X]";
        }
        
        return PhrasePiece(subphrase.str());
    }
    
    bool IsHierarchical() const {
        return m_hierarchical;
    }
    
    bool operator<(const PhrasePiece& other) const {
        return m_phraseString < other.m_phraseString;
    }
    
    const Phrase& GetMosesPhrase() const {
        assert(m_mosesPhrase);
        return *m_mosesPhrase;
    }

protected:
    void Init()
    {
        Tokenize(m_phraseString, m_phraseTokens);
        if(!m_phraseTokens.empty()) {
            StringPiece &last = m_phraseTokens.back();
            size_t size = last.size();
            
            if(last.data()[0] == '[' && last.data()[size-1] == ']')
                m_hierarchical = true;
        }
    }
    
    void Tokenize(const StringPiece& str,
                  std::vector<StringPiece>& out)
    {
        const char* seps = " \t";
        StringPiece suffix = str;
        size_t len = suffix.length();
        size_t span;
        while((span = strcspn(suffix.data(), seps)) != len)
        {
            out.push_back(StringPiece(suffix.data(), span));
            size_t skip = 0;
            char* ptr = (char*)suffix.data() + span;
            while(strpbrk(ptr, seps) == ptr)
            {
                ptr++;
                skip++;
            }
            len = len - span - skip;
            suffix = StringPiece(suffix.data() + span + skip, len);
        }
        if(len)
            out.push_back(suffix);
    }

    const std::string m_phraseString;
    std::vector<StringPiece> m_phraseTokens;
    const Phrase *m_mosesPhrase;
    bool m_hierarchical;
};

}
#endif
