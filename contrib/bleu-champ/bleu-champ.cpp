
#include <iostream>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>

#include "util/exception.hh"

#include "Dynamic.hpp"
#include "Scorer.hpp"
#include "Corpus.hpp"
#include "Printer.hpp"

namespace po = boost::program_options;

template <class Config, class Corpus>
Ladder FirstPass(Corpus &source, Corpus &target, bool quiet) {
  boost::timer::auto_cpu_timer t(std::cerr, 2, "    Time: %t sec CPU, %w sec real\n");
  if(!quiet) std::cerr << "Pass 1: Tracing path with beads:";
  if(!quiet) {
    for(const Bead& bead : Config().Search()())
      std::cerr << " " << bead;
    std::cerr << std::endl;
  }
  if(!quiet) std::cerr << "    Computing best path" << std::endl;
  Dynamic<Config, Corpus> aligner(source, target);
  aligner.Align();
  if(!quiet) std::cerr << "    Back-tracking" << std::endl;
  Ladder path = aligner.BackTrack();
  t.stop();
  if(!quiet) t.report();
  if(!quiet) std::cerr << std::endl;
  return path;
}

template <class Config, class Corpus>
Ladder SecondPass(Corpus &source, Corpus &target, const Ladder& path, size_t corridorWidth, bool quiet) {
  boost::timer::auto_cpu_timer t(std::cerr, 2, "    Time: %t sec CPU, %w sec real\n");
  if(!quiet) std::cerr << "Pass 2: Tracing path with beads:";
  if(!quiet) {
    for(const Bead& bead : Config().Search()())
      std::cerr << " " << bead;
    std::cerr << std::endl;
  }
  if(!quiet) std::cerr << "    Setting corridor width to " << corridorWidth << std::endl;
  Dynamic<Config, Corpus> aligner(source, target);
  aligner.SetCorridor(path, corridorWidth);
  if(!quiet) std::cerr << "    Computing best path within corridor" << std::endl;
  aligner.Align();
  if(!quiet) std::cerr << "    Back-tracking" << std::endl;
  Ladder rungs = aligner.BackTrack();
  t.stop();
  if(!quiet) t.report();
  if(!quiet) std::cerr << std::endl;
  return rungs;
}

int main(int argc, char** argv)
{
  bool help;
  bool skip1st;
  bool skip2nd;
  bool quiet;
  
  std::string sourceFileName;
  std::string targetFileName;

  std::string sourceFileNameOrig;
  std::string targetFileNameOrig;

  bool ladderFormat;
  size_t corridorWidth;
  
  PrintParams params;
  
  po::options_description general("General options");
  general.add_options()
    ("source,s", po::value<std::string>(&sourceFileName)->required(),
     "Source language file, used for alignment computation")
    ("target,t", po::value<std::string>(&targetFileName)->required(),
     "Target language file, used for alignment computation")
    ("help,h", po::value(&help)->zero_tokens()->default_value(false),
     "Print this help message and exit")
    ("quiet,q", po::value(&quiet)->zero_tokens()->default_value(false),
     "Do not print anything to stderr")
  ;

  po::options_description algo("Alignment algorithm options");
  algo.add_options()
    ("width,w", po::value(&corridorWidth)->default_value(30),
     "Width of search corridor around 1-1 path")
    ("skip-1st", po::value(&skip1st)->zero_tokens()->default_value(false),
     "Skip 1st pass. Can be very slow for larger files")
    ("skip-2nd", po::value(&skip2nd)->zero_tokens()->default_value(false),
     "Skip 2nd pass and output only 1-1 path")
  ;

  po::options_description output("Output options");
  output.add_options()
  
    ("ladder,l", po::value(&ladderFormat)->zero_tokens()->default_value(false),
     "Output in hunalign ladder format (not affected by other printing options)")

    ("Source,S", po::value<std::string>(&sourceFileNameOrig),
     "Substitute source language file, used only for output in text mode. "
     "Has to be sentence aligned with --source  arg")
    ("Target,T", po::value<std::string>(&targetFileNameOrig),
     "Substitute target language file, used only for output in text mode. "
     "Has to be sentence aligned with --target  arg")

     
    ("min-score,m", po::value(&params.printThreshold)->default_value(0),
     "Print rungs with scores of at least  arg")
    
    ("print-beads,b", po::value(&params.printBeads)->zero_tokens()->default_value(false),
     "Print column with beads")
    ("print-ids,i", po::value(&params.printIds)->zero_tokens()->default_value(false),
     "Print column with sentence ids")
    ("print-scores,p", po::value(&params.printScores)->zero_tokens()->default_value(false),
     "Print column with scores")
    ("print-1-1,1", po::value(&params.print11)->zero_tokens()->default_value(false),
     "Print only 1-1 rungs")
    ("print-unaligned,u", po::value(&params.printUnaligned)->zero_tokens()->default_value(false),
     "Print unaligned sentences")
  ;

  po::options_description cmdline_options("Allowed options");
  cmdline_options.add(general).add(algo).add(output);
  po::variables_map vm;
  
  try { 
    po::store(po::command_line_parser(argc,argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);
  }
  catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl << std::endl;
    
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << cmdline_options << std::endl;
    exit(0);
  }
  
  if (help) {
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << cmdline_options << std::endl;
    exit(0);
  }

  boost::timer::auto_cpu_timer t(std::cerr, 2, "Total time: %t sec CPU, %w sec real\n");
  
  if(!quiet) std::cerr << std::endl;
  std::shared_ptr<Corpus> source(new Corpus(sourceFileName));
  if(!quiet) std::cerr << "Loaded " << source->size() << " source sentences" << std::endl;
  std::shared_ptr<Corpus> target(new Corpus(targetFileName));
  if(!quiet) std::cerr << "Loaded " << target->size() << " target sentences" << std::endl;
  if(!quiet) std::cerr << std::endl;
  
  Ladder rungsMN;

  if(skip2nd) {
    rungsMN = FirstPass<Config<BLEU<2>, Fast>, Corpus>(*source, *target, quiet);
  }
  else if(skip1st) {
    rungsMN = FirstPass<Config<BLEU<2>, Full>, Corpus>(*source, *target, quiet);    
  }
  else {
    Ladder rungs11 = FirstPass<Config<BLEU<2>, Fast>, Corpus>(*source, *target, quiet);
    rungsMN = SecondPass<Config<BLEU<2>, Full>, Corpus>(*source, *target, rungs11, corridorWidth, quiet);
  }  

  t.stop();
  if(!quiet) t.report();
  if(!quiet) std::cerr << std::endl;
  
  if(sourceFileNameOrig.size())
    source.reset(new Corpus(sourceFileNameOrig));
  if(targetFileNameOrig.size())
    target.reset(new Corpus(targetFileNameOrig));
    
  if(ladderFormat) {
    Print<LadderFormat>(rungsMN, *source, *target, params);
  }
  else {
    Print<TextFormat>(rungsMN, *source, *target, params);
  }
  
  if(!quiet)
    PrintStatistics(rungsMN);
}
