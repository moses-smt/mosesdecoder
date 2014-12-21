#pragma once

#include <vector>
#include <iostream>
#include <algorithm> 

/******************************************************************************/

struct Bead {
  Bead() : m_bead{0 ,0} {}
  
  Bead(size_t i, size_t j) : m_bead{i, j} {}
  
  inline size_t& operator[](size_t i) {
    UTIL_THROW_IF(i > 1, util::Exception, "Error: Only two elements in bead.");
    return m_bead[i];
  }

  inline const size_t& operator[](size_t i) const {
    return const_cast<Bead&>(*this)[i];
  }
  
  size_t m_bead[2];
};

std::ostream& operator<<(std::ostream &o, Bead bead) {
  return o << bead[0] << "-" << bead[1];
}

typedef std::vector<Bead> Beads;

struct Rung {
  size_t i;
  size_t j;
  Bead bead;
  float score;
};

typedef std::vector<Rung> Ladder;

/******************************************************************************/

template <Beads& allowedBeads>
struct Search {
  public:
    Search()
    : m_allowedBeads(allowedBeads)
    {
      UTIL_THROW_IF(m_allowedBeads.size() < 1, util::Exception,
                    "Error: You have to define at least one search bead.");
    }
    
    virtual const Beads& operator()() const {
      return m_allowedBeads;
    }
  
  private:
    Beads& m_allowedBeads;
};

Beads fastBeads = {{0,1}, {1,0}, {1,1}};
typedef Search<fastBeads> Fast;

Beads fullBeads = {{0,1}, {1,0}, {1,1}, {1,2}, {2,1}, {2,2},
                   {1,3}, {3,1}, {2,3}, {3,2}, {3,3}, {1,4},
                   {2,4}, {3,4}, {4,3}, {4,2}, {4,1}};
typedef Search<fullBeads> Full;

/******************************************************************************/

template <class ScorerType, class SearchType>
class Config {
  public:
    const ScorerType& Scorer() const {
      return m_scorer;
    }
    
    const SearchType& Search() const {
      return m_search;
    }
    
  private:
    ScorerType m_scorer;
    SearchType m_search;
};

/******************************************************************************/

template<class ConfigType, class CorpusType>
class Dynamic {
  public:
    Dynamic(CorpusType& corpus1, CorpusType& corpus2)
    : m_corpus1(corpus1), m_corpus2(corpus2),
      m_seen(m_corpus1.size() + 1, std::vector<float>(m_corpus2.size() + 1, -100)),
      m_prev(m_corpus1.size() + 1, std::vector<Bead>(m_corpus2.size() + 1))
    {}
    
    void Align() {
      Align(m_corpus1.size(), m_corpus2.size());
    }
    
    float Align(int i, int j) {
      if(i <= 0 && j <= 0)
        return 0;
      
      if(m_seen[i][j] != -100)
        return m_seen[i][j];
      
      Beads allowedBeads = m_config.Search()();
      
      float bestScore = -1;
      Bead bestBead = allowedBeads[0];
      
      for(Bead& bead : allowedBeads) {
        float score = -10;
        if(i >= bead[0] && j >= bead[1] && InCorridor(i - bead[0], j - bead[1])) {
          score = Align(i - bead[0], j - bead[1])
                   + m_config.Scorer()(m_corpus1(i - bead[0], i - 1),
                                       m_corpus2(j - bead[1], j - 1));
        
          if(score > bestScore) {
            bestScore = score;
            bestBead = bead;
          }
        }
      }
      
      m_seen[i][j] = bestScore;
      m_prev[i][j] = bestBead;
      return bestScore;    
    }

    bool InCorridor(size_t i, size_t j) {
      if(!m_corridor.empty()) {
        //std::cout << "Corr: " << i << " " << j << std::endl;
        return m_corridor[i][j];
      }
      return true;
    }
  
    Ladder BackTrack() {
      Ladder ladder;
      BackTrack(m_corpus1.size(), m_corpus2.size(), ladder);
      
      Rung final;
      final.i = m_corpus1.size();
      final.j = m_corpus2.size();
      final.score = 0.0;
      ladder.push_back(final);
      
      return ladder;
    }
  
    void BackTrack(size_t i, size_t j, Ladder& ladder) {
      if(i == 0 && j == 0)
        return;
      
      Bead bead = m_prev[i][j];
      BackTrack(i - bead[0], j - bead[1], ladder);
      
      Rung rung;
      rung.i = i - bead[0];
      rung.j = j - bead[1];
      rung.bead = bead;
      
      if(m_seen[i - bead[0]][j - bead[1]] != -100)
        rung.score = m_seen[i][j] - m_seen[i - bead[0]][j - bead[1]];
      else
        rung.score = 0;
        
      ladder.push_back(rung);
    }
    
    void SetCorridor(const Ladder& ladder, size_t width = 20) {
      UTIL_THROW_IF(ladder.empty(), util::Exception,
                    "Error: No elements in ladder.");
      
      size_t m = ladder.back().i;
      size_t n = ladder.back().j;
      
      int distance = width/2;
      m_corridor.resize(m + 1, std::vector<bool>(n + 1, false));
      for(const Rung& r : ladder) {
        for(int j = std::max(0, (int)r.j - distance); j < std::min((int)r.j + distance, (int)n); j++)
          m_corridor[r.i][j] = true;
      }
    }
    
  private:
    ConfigType m_config;
    CorpusType& m_corpus1;
    CorpusType& m_corpus2;
        
    std::vector< std::vector<float> > m_seen;
    std::vector< std::vector<Bead> > m_prev;
    
    std::vector< std::vector<bool> > m_corridor;
};
