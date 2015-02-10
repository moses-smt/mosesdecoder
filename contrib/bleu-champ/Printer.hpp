#pragma once

#include <vector>
#include <map>
#include <cstdio>
#include <limits>

#include "Dynamic.hpp"

struct PrintParams {
  bool printIds = false;
  bool printBeads  = false;
  bool printScores = false;
  bool printUnaligned = false;
  bool print11 = false;
  float printThreshold = MIN;
};

struct TextFormat {
  template <class Corpus>
  static void Print(const Rung& r, const Corpus& source, const Corpus& target,
                    const PrintParams& params) {
    if(r.i == source.size() && r.j == target.size())
      return;
    
    if(r.score < params.printThreshold)
      return;
    if(params.print11 && (r.bead[0] != 1 || r.bead[1] != 1))
      return;
    if(!params.printUnaligned && (r.bead[0] == 0 || r.bead[1] == 0))
      return;
  
    const Sentence& s1 = source(r.i, r.i + r.bead[0] - 1);
    const Sentence& s2 = target(r.j, r.j + r.bead[1] - 1);
    
    if(params.printIds)    std::cout << r.i << " " << r.j << "\t";
    if(params.printBeads)  std::cout << r.bead << "\t";
    if(params.printScores) std::cout << r.score <<  "\t";
    
    std::cout << s1 << "\t" << s2 << std::endl;
  }
};

struct LadderFormat {
  template <class Corpus>
  static void Print(const Rung& r, const Corpus& source, const Corpus& target,
                    const PrintParams& params) {
    std::cout << r.i << "\t" << r.j << "\t" << r.score << std::endl;
  }
};

template <class Format, class Corpus>
void Print(const Ladder& ladder, const Corpus& source, const Corpus& target,
           const PrintParams& params) {
  for(const Rung& rung : ladder) {
    Format::Print(rung, source, target, params);
  }
}

void PrintStatistics(const Ladder& ladder) {
  std::map<Bead, size_t> stats;
  
  size_t nonZero = 0;
  float scoreSum = 0;
  for(size_t i = 0; i < ladder.size()-1; i++) {
    const Rung& r = ladder[i]; 
    stats[r.bead]++;
    if(r.bead[0] > 0 && r.bead[1] > 0) {
      scoreSum += r.score;
      nonZero++;
    }
  }  

  std::cerr << "Bead statistics: " << std::endl;
  for(auto& item : stats) {
    float percent = ((float)item.second/(ladder.size()-1)) * 100;
    fprintf(stderr, "    %lu-%lu : %4lu (%5.2f\%)\n", item.first[0], item.first[1], item.second, percent);
  }
  std::cerr << std::endl;
 
  std::cerr << "Quality (aligned): " << scoreSum/nonZero << std::endl;
  std::cerr << "Quality (total): " << scoreSum/(ladder.size()-1) << std::endl;
  std::cerr << std::endl;  
}

