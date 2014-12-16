
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "util/exception.hh"

#include "Scorer.h"
#include "ScorerFactory.h"

using namespace MosesTuning;

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  bool help;
  std::string sctype = "BLEU";
  std::string scconfig = "";
  std::string source;
  std::string target;

  // Command-line processing follows pro.cpp
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
    ("sctype", po::value<std::string>(&sctype), "the scorer type (default BLEU)")
    ("scconfig,c", po::value<std::string>(&scconfig), "configuration string passed to scorer")
    ("source,s", po::value<std::string>(&source)->required(), "source language file")
    ("target,t", po::value<std::string>(&target)->required(), "target language file")
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
  
  boost::scoped_ptr<Scorer> scorerFwd(ScorerFactory::getScorer(sctype, scconfig));
  std::vector<std::string> targetFiles;
  targetFiles.push_back(target);
  scorerFwd->setReferenceFiles(targetFiles);
  
  boost::scoped_ptr<Scorer> scorerBwd(ScorerFactory::getScorer(sctype, scconfig));
  std::vector<std::string> sourceFiles;
  sourceFiles.push_back(source);
  scorerBwd->setReferenceFiles(sourceFiles);  
  
  std::ifstream sourceStream(source.c_str());
  if (!sourceStream.good())
    throw std::runtime_error("Error opening candidate file");
  std::vector<std::string> sourceLines;  
  //ScoreStats scoreEntry;
  std::string line;
  while (std::getline(sourceStream, line)) {
    //scorerFwd->prepareStats(entries.size(), line, scoreEntry);
    //entries.push_back(scoreEntry);
    sourceLines.push_back(line);
  }
  
  std::ifstream targetStream(target.c_str());
  if (!targetStream.good())
    throw std::runtime_error("Error opening candidate file");
  //std::vector<ScoreStats> entries;
  std::vector<std::string> targetLines;  
  while (std::getline(targetStream, line)) {
    targetLines.push_back(line);
  }
  
  std::cout << sourceLines.size() << std::endl;
  std::cout << targetLines.size() << std::endl;
  
  for(int i = 0; i < sourceLines.size(); i++) {
    for(int j = 0; j < targetLines.size(); j++) {
      ScoreStats scoreEntry;
      scorerFwd->prepareStats(j, sourceLines[i], scoreEntry);  
      std::cout << i << " " << j << " : " << scoreEntry << std::endl;
    }
  }
}
