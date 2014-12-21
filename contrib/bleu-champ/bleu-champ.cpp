
#include <iostream>
#include <vector>
#include <map>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>

#include "util/exception.hh"

#include "Dynamic.hpp"
#include "Scorer.hpp"
#include "Corpus.hpp"

namespace po = boost::program_options;

template <class Config, class Corpus>
Ladder FirstPass(Corpus &source, Corpus &target) {
  std::cerr << " Pass 1: Tracing path with 1-1 beads:" << std::endl;
  std::cerr << "         Computing best path" << std::endl;
  Dynamic<Config, Corpus> aligner(source, target);
  aligner.Align();
  std::cerr << "         Back-tracking" << std::endl;
  Ladder path = aligner.BackTrack();
  std::cerr << "         Done" << std::endl;
  std::cerr << std::endl;
  return path;
}

template <class Config, class Corpus>
Ladder SecondPass(Corpus &source, Corpus &target, const Ladder& path, size_t corridorWidth) {
  std::cerr << " Pass 2: Tracing path with all beads:" << std::endl;
  std::cerr << "         Setting corridor width to " << corridorWidth << std::endl;
  Dynamic<Config, Corpus> aligner(source, target);
  aligner.SetCorridor(path, corridorWidth);
  std::cerr << "         Computing best path within corridor" << std::endl;
  aligner.Align();
  std::cerr << "         Back-tracking" << std::endl;
  Ladder rungs = aligner.BackTrack();
  std::cerr << "         Done" << std::endl;
  std::cerr << std::endl;
  return rungs;
}

struct PrintParams {
  bool printIds = false;
  bool printBeads  = false;
  bool printScores = false;
  bool printUnaligned = false;
  bool print11 = false;
  float printThreshold = 0;
};

struct TextFormat {
  static void Print(const Rung& r, const Corpus& source, const Corpus& target, const PrintParams& params) {
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
  static void Print(const Rung& r, const Corpus& source, const Corpus& target, const PrintParams& params) {
    std::cout << r.i << "\t" << r.j << "\t" << r.score << std::endl;
  }
};

template <class Format, class Corpus>
void Print(const Ladder& ladder, const Corpus& source, const Corpus& target, const PrintParams& params) {
  for(const Rung& rung : ladder) {
    Format::Print(rung, source, target, params);
  }
}

int main(int argc, char** argv)
{
  boost::timer::auto_cpu_timer t(std::cerr);
  
  bool help;
  
  std::string sourceFileName;
  std::string targetFileName;

  std::string sourceFileNameOrig;
  std::string targetFileNameOrig;

  bool ladderFormat;
  size_t corridorWidth;
  
  PrintParams params;
  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("source,s", po::value<std::string>(&sourceFileName)->required(),
     "Source language file, used for alignment computation")
    ("target,t", po::value<std::string>(&targetFileName)->required(),
     "Target language file, used for alignment computation")
    
    ("Source,S", po::value<std::string>(&sourceFileNameOrig),
     "Substitute source language file, if given will replace output of --source")
    ("Target,T", po::value<std::string>(&targetFileNameOrig),
     "Substitute target language file, if given will replace output of --target")

    ("width,w", po::value(&corridorWidth)->default_value(30),
     "Width of search corridor around 1-1 path")
    
    ("ladder,l", po::value(&ladderFormat)->zero_tokens()->default_value(false),
     "Output in hunalign ladder format (not affected by other printing options)")

    ("min-score,m", po::value(&params.printThreshold)->default_value(0),
     "Print rungs with scores of at least  arg")
    
    ("print-beads,b", po::value(&params.printBeads)->zero_tokens()->default_value(false),
     "Print column of beads")
    ("print-ids,i", po::value(&params.printIds)->zero_tokens()->default_value(false),
     "Print column of sentence ids")
    ("print-scores,p", po::value(&params.printScores)->zero_tokens()->default_value(false),
     "Print column of scores")
    ("print-1-1,1", po::value(&params.print11)->zero_tokens()->default_value(false),
     "Print only 1-1 rungs")
    ("print-unaligned,u", po::value(&params.printUnaligned)->zero_tokens()->default_value(false),
     "Print unaligned sentences")
    
    ("help,h", po::value(&help)->zero_tokens()->default_value(false),
     "Print this help message and exit")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  
  try { 
    po::store(po::command_line_parser(argc,argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);
  }
  catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl << std::endl;
    
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << desc << std::endl;
    exit(0);
  }
  
  if (help) {
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << desc << std::endl;
    exit(0);
  }
  
  std::cerr << std::endl;
  
  Corpus source(sourceFileName);
  std::cerr << " Loaded " << source.size() << " source sentences" << std::endl;
  Corpus target(targetFileName);
  std::cerr << " Loaded " << target.size() << " target sentences" << std::endl;
  std::cerr << std::endl;

  Ladder rungs11 = FirstPass<Config<BLEU<2>, Fast>, Corpus>(source, target);
  Ladder rungsMN = SecondPass<Config<BLEU<2>, Full>, Corpus>(source, target, rungs11, corridorWidth);
  
  if(ladderFormat) {
    Print<LadderFormat>(rungsMN, source, target, params);
  }
  else {
    Print<TextFormat>(rungsMN, source, target, params);
  }
  
  //float scoreSum = 0;
  //size_t keptRungs = 0;
  //
  //std::map<std::pair<size_t,size_t>, size_t> stats;
  //
  //      stats[std::make_pair(r.bead[0], r.bead[1])]++;
  //
  //std::cerr << " Bead statistics: " << std::endl;
  //for(auto& item : stats)
  //  std::cerr << "   " << item.first.first << "-" << item.first.second << " : " << item.second << std::endl;
  //  
  //std::cerr << std::endl;
  //std::cerr << " Quality of aligned rungs: " << scoreSum/keptRungs << std::endl;
  //std::cerr << " Quality: " << scoreSum/rungs.size() << std::endl;
  //std::cerr << std::endl;
}
