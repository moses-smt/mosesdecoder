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

#ifndef moses_ConsistentPhrases_h
#define moses_ConsistentPhrases_h

#include <set>

namespace Moses
{

class ConsistentPhrases
{
public:
  struct Phrase {
    int i, j, m, n;
    Phrase(int i_, int m_, int j_, int n_) : i(i_), j(j_), m(m_), n(n_) { }
  };

  struct PhraseSorter {
    bool operator()(Phrase a, Phrase b) {
      if(a.n > b.n)
        return true;
      if(a.n == b.n && a.j < b.j)
        return true;
      if(a.n == b.n && a.j == b.j && a.m > b.m)
        return true;
      if(a.n == b.n && a.j == b.j && a.m == b.m && a.i < b.i)
        return true;
      return false;
    }
  };

private:
  typedef std::set<Phrase, PhraseSorter> PhraseQueue;
  PhraseQueue m_phraseQueue;

  typedef std::pair<unsigned char, unsigned char> AlignPoint;
  typedef std::set<AlignPoint> Alignment;

public:

  ConsistentPhrases(int mmax, int nmax, Alignment& a) {
    for(int i = 0; i < mmax; i++) {
      for(int m = 1; m <= mmax-i; m++) {
        for(int j = 0; j < nmax; j++) {
          for(int n = 1; n <= nmax-j; n++) {
            bool consistant = true;
            for(Alignment::iterator it = a.begin(); it != a.end(); it++) {
              int ip = it->first;
              int jp = it->second;
              if((i <= ip && ip < i+m) != (j <= jp && jp < j+n)) {
                consistant = false;
                break;
              }
            }
            if(consistant)
              m_phraseQueue.insert(Phrase(i, m, j, n));
          }
        }
      }
    }
    m_phraseQueue.erase(Phrase(0, mmax, 0, nmax));
  }

  size_t Empty() {
    return !m_phraseQueue.size();
  }

  Phrase Pop() {
    if(m_phraseQueue.size()) {
      Phrase p = *m_phraseQueue.begin();
      m_phraseQueue.erase(m_phraseQueue.begin());
      return p;
    }
    return Phrase(0,0,0,0);
  }

  void RemoveOverlap(Phrase p) {
    PhraseQueue ok;
    for(PhraseQueue::iterator it = m_phraseQueue.begin(); it != m_phraseQueue.end(); it++) {
      Phrase pp = *it;
      if(!((p.i <= pp.i && pp.i < p.i + p.m) || (pp.i <= p.i && p.i < pp.i + pp.m) ||
           (p.j <= pp.j && pp.j < p.j + p.n) || (pp.j <= p.j && p.j < pp.j + pp.n)))
        ok.insert(pp);
    }
    m_phraseQueue = ok;
  }

};

}

#endif
