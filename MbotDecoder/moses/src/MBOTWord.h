// $Id: MBOTWord.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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

#ifndef moses_MBOTWord_h
#define moses_MBOTWord_h

#include <cstring>
#include <iostream>
#include <vector>
#include <list>
#include "Word.h"
#include "TypeDef.h"
#include "Factor.h"
#include "Util.h"

namespace Moses {

/***
* Holds a vector of words
*/

class MBOTWord
{
 public :
 std::vector<Word> m_words;
 std::vector<const Factor*> * m_factors;
  // iters
typedef std::vector<Word>::iterator iterator;
typedef std::vector<Word>::const_iterator const_iterator;

 MBOTWord()
 {
     m_factors = new std::vector<const Factor*>;
 }

 ~MBOTWord()
 {
    //std::cout << "DELETING MBOT WORD" << std::endl;
    //delete m_factors;
 }

const_iterator begin() {
    return m_words.begin();
  }

  const_iterator end() {
    return m_words.end();
  }

 const std::vector<Word> GetWordVector() const
 {
     return m_words;
 }

 void SetWordVector(std::vector<Word> wv)
 {
    m_words = wv;
 }

 size_t GetSize() const
 {
     return m_words.size();
 }

  const std::vector<const Factor*> GetFactors() const
 {
     //std::cout << "MBOT WORD : GETTING FACTORS " << std::endl;
     std::vector<Word> :: const_iterator itr_words;
     for(itr_words = m_words.begin();itr_words != m_words.end();itr_words++)
     {
        //std::cout << "MBOT Word : " << *itr_words << std::endl;
        //std::cout << "MBOT try to get factor : " << std::endl;
        const Factor *f = (*itr_words)[0];
        //std::cout << "MBOT factor here : " << std::endl;
        m_factors->push_back((*itr_words)[0]);
        //std::cout << "MBOT factor pushed back : " << std::endl;
     }

    //std::cout << "BEFORE RETURN" << std::endl;
    //PROBLEM WITH RETURN
    return *m_factors;
 }

 //! transitive comparison MBOTWORD objects
  inline bool operator < (MBOTWord compare) {
    if(m_words.size() != compare.GetSize())
    {
        return m_words.size() < compare.GetSize();
    }
    else
    {
        std::vector<Word> :: const_iterator itr_words;
        std::vector<Word> :: const_iterator itr_compare;
        for(itr_words = m_words.begin(), itr_compare = compare.begin();
            itr_words != end(), itr_compare != compare.end();
            itr_words++, itr_compare++)
        {
            if((*itr_words) != (*itr_compare))
            {
                return Word::Compare(*itr_words, *itr_compare) < 0;
            }
        }
    }
  }

  inline bool operator== (MBOTWord compare) {
    if(m_words.size() != compare.GetSize())
    {
        return 0;
    }
    else
    {
        std::vector<Word> :: const_iterator itr_words;
        std::vector<Word> :: const_iterator itr_compare;
        for(itr_words = m_words.begin(), itr_compare = compare.begin();
            itr_words != m_words.end(), itr_compare != compare.end();
            itr_words++, itr_compare++)
        {
            if(*itr_words != *itr_compare)
            {
                return 0;
            }
        }
    }
}

  inline bool operator!= (MBOTWord compare) {
      if(GetSize() != compare.GetSize())
    {
        return 1;
    }
    else
    {
        std::vector<Word> :: const_iterator itr_words;
        std::vector<Word> :: const_iterator itr_compare;
        for(itr_words = m_words.begin(), itr_compare = compare.begin();
            itr_words != m_words.end(), itr_compare != compare.end();
            itr_words++, itr_compare++)
        {
            //std::cout << "First MBOT word : " << *itr_words << std::endl;
            //std::cout << "Second MBOT word : " << *itr_words << std::endl;

            const Factor * f1 = (*itr_words)[0];
            //std::cout << "First factor : " << *f1 << std::endl;
            const Factor * f2 = (*itr_compare)[0];
            //std::cout << "Second factor : " << *f1 << std::endl;

             if (f1->Compare(*f2)) {
                //std::cout << "Target Words are not the same !!!" << std::endl;
            return 1;
             }
        }
        return 0;
    }
  }
};

}//namespace

#endif
