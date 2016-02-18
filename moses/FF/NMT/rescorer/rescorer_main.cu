#include <iostream>
#include <string>
#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "nbest.h"
#include "vocab.h"
#include "rescorer.h"
#include "model.h"

void ProgramOptions(int argc, char *argv[],
    std::string& modelPath,
    std::string& svPath,
    std::string& tvPath,
    std::string& corpusPath,
    std::string& nbestPath,
    std::string& fname,
    size_t& maxBatchSize,
    size_t& device) {
  bool help = false;

  namespace po = boost::program_options;
  po::options_description cmdline_options("Allowed options");
  cmdline_options.add_options()
    ("device,d", po::value(&device)->default_value(0),
     "CUDA Device")
    ("batch,b", po::value(&maxBatchSize)->default_value(1000),
     "Max batch size")
    ("model,m", po::value(&modelPath)->required(),
     "Path to a model")
    ("source,s", po::value(&svPath)->required(),
     "Path to a source vocab file.")
    ("target,t", po::value(&tvPath)->required(),
     "Path to a target vocab file.")
    ("input,i", po::value(&corpusPath)->required(),
     "Path to the input of the nbest file.")
    ("n-best,n", po::value(&nbestPath)->required(),
     "Path to an nbest file.")
    ("feature-name,f", po::value(&fname)->default_value("NMT0"),
     "Feature name")
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
  std::string modelPath, svPath, tvPath, corpusPath, nbestPath, fname;

  size_t device;
  size_t maxBatchSize;
  ProgramOptions(argc, argv, modelPath, svPath,tvPath, corpusPath, nbestPath,
                 fname, maxBatchSize, device);
  cudaSetDevice(device);

  std::cerr << "Loading model: " << modelPath << std::endl;
  std::shared_ptr<Weights> weights(new Weights(modelPath, device));
  std::shared_ptr<Vocab> srcVocab(new Vocab(svPath));
  std::shared_ptr<Vocab> trgVocab(new Vocab(tvPath));

  std::cerr << "Loading nbest list: " << nbestPath << std::endl;
  std::shared_ptr<NBest> nbest(new NBest(corpusPath,nbestPath, srcVocab, trgVocab, maxBatchSize));

  std::cerr << "Creating rescorer..." << std::endl;
  std::shared_ptr<Rescorer> rescorer(new Rescorer(weights, nbest, fname));

  std::cerr << "Start rescoring..." << std::endl;
  for (size_t i = 0; i < nbest->size(); ++i) {
    rescorer->Score(i);
  }

  return 0;
}
