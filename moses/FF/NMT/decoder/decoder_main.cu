#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <memory>
#include <boost/timer/timer.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/lexical_cast.hpp>

#include "bahdanau/model.h"
#include "vocab.h"
#include "decoder/nmt_decoder.h"


void ProgramOptions(int argc, char *argv[],
    std::string& modelPath,
    std::string& svPath,
    std::string& tvPath,
    size_t& device) {
  bool help = false;

  namespace po = boost::program_options;
  po::options_description cmdline_options("Allowed options");
  cmdline_options.add_options()
    ("device,d", po::value(&device)->default_value(0),
     "CUDA Device")
    ("model,m", po::value(&modelPath)->required(),
     "Path to a model")
    ("source,s", po::value(&svPath)->required(),
     "Path to a source vocab file.")
    ("target,t", po::value(&tvPath)->required(),
     "Path to a target vocab file.")
    ("help,h", po::value(&help)->zero_tokens()->default_value(false),
     "Print this help message and exit.")
  ;
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
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
}

int main(int argc, char* argv[]) {
  std::string modelPath, srcVocabPath, trgVocabPath;
  size_t device = 0;
  ProgramOptions(argc, argv, modelPath, srcVocabPath, trgVocabPath, device);
  std::cerr << "Using device GPU" << device << std::endl;;
  cudaSetDevice(device);
  std::cerr << "Loading model... ";
  std::shared_ptr<Weights> model(new Weights(modelPath));
  std::shared_ptr<Vocab> srcVocab(new Vocab(srcVocabPath));
  std::shared_ptr<Vocab> trgVocab(new Vocab(trgVocabPath));
  std::cerr << "done." << std::endl;

  NMTDecoder decoder(model, srcVocab, trgVocab);

  std::cerr << "Start translating...\n";

  std::ios_base::sync_with_stdio(false);

  std::string line;
  boost::timer::cpu_timer timer;
  while(std::getline(std::cin, line)) {
    auto result = decoder.translate(line);
    for (auto it = result.rbegin(); it != result.rend(); ++it) std::cout << (*trgVocab)[*it] << " ";
    std::cout << std::endl;
  }
  std::cerr << timer.format() << std::endl;
  return 0;
}
