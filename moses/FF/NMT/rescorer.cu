
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/lexical_cast.hpp>

#include "mblas/matrix.h"
#include "model.h"
#include "encoder.h"
#include "decoder.h"
#include "vocab.h"

#include "states.h"

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

void ParseInputFile(std::string path, std::vector<std::string>& output) {
    std::ifstream file(path);
    output.clear();
    std::string line;
    while (std::getline(file, line).good()) {
      line += " </s>";
      output.push_back(line);
    }
}

std::vector<std::string> Split(std::string& line, std::string del=" ") {
    std::vector<std::string> output;
    size_t pos = 0;
    std::string token;
    while ((pos = line.find(del)) != std::string::npos) {
        token = line.substr(0, pos);
        output.push_back(token);
        line.erase(0, pos + del.size());
    }
    output.push_back(line);
    return output;
}

void ParseNBestFile(std::string path, std::vector<std::vector<std::string>>& output) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line).good()) {
        output.push_back(Split(line, " ||| "));
    }
}

void PrepareBatch(const std::vector<std::vector<size_t>>& input,
                  std::vector<std::vector<size_t>>& output) {

    size_t maxSentenceLength = 0;
    for (auto& sentence: input) {
        maxSentenceLength = max(maxSentenceLength, sentence.size());
    }

    for (size_t i = 0; i < maxSentenceLength; ++i) {
        output.emplace_back(input.size(), 0);
        for (size_t j = 0; j < input.size(); ++j) {
            if (i < input[j].size()) {
                output[i][j] = input[j][i];
            }
        }
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
  Weights weights(modelPath, device);
  Vocab svcb(svPath);
  Vocab tvcb(tvPath);

  std::vector<std::string> input;
  ParseInputFile(corpusPath, input);

  std::vector<std::vector<std::string>> nbest;
  ParseNBestFile(nbestPath, nbest);

  size_t index = 0;
  size_t nbestIndex = 0;
  for (auto& in: input) {
    Encoder encoder(weights);
    Decoder decoder(weights);

    auto words = Split(in);
    auto sIndexes = svcb.Encode(words);

    std::vector<std::vector<size_t> > sentences2score;
    while (nbestIndex < nbest.size()) {
      sentences2score.clear();
      for (; (nbestIndex < nbest.size()) && (sentences2score.size() < maxBatchSize); ++nbestIndex) {
        if (boost::lexical_cast<size_t>(nbest[nbestIndex][0]) == index) {
          std::string sentence = nbest[nbestIndex][1] + " </s>";
          sentences2score.push_back(tvcb.Encode(Split(sentence)));
        }
        else {
          break;
        }

      }
      if (sentences2score.size() == 0 ) {
        index = boost::lexical_cast<size_t>(nbest[nbestIndex][0]);
        continue;
      }

      std::vector<std::vector<size_t>> batch;
      PrepareBatch(sentences2score, batch);

      if(index > 0 && index % 5 == 0)
        std::cerr << ".";
      if(index > 0 && index % 100 == 0)
        std::cerr << "[" << index << "]" << std::endl;

      mblas::Matrix SourceContext;
      encoder.GetContext(sIndexes, SourceContext);

      mblas::Matrix PrevState;
      mblas::Matrix PrevEmbedding;

      mblas::Matrix AlignedSourceContext;
      mblas::Matrix Probs;

      mblas::Matrix State;
      mblas::Matrix Embedding;
      size_t batchSize = batch[0].size();

      decoder.EmptyState(PrevState, SourceContext, batchSize);
      decoder.EmptyEmbedding(PrevEmbedding, batchSize);

      std::vector<float> scores(batch[0].size(), 0.0f);
      size_t lengthIndex = 0;
      for (auto& w : batch) {
        decoder.GetProbs(Probs, AlignedSourceContext,
                         PrevState, PrevEmbedding, SourceContext);

        for (size_t j = 0; j < w.size(); ++j) {
          if (batch[lengthIndex][j]) {
            float p = Probs(j, w[j]);
            scores[j] += log(p);
          }
        }

        decoder.Lookup(Embedding, w);
        decoder.GetNextState(State, Embedding,
                             PrevState, AlignedSourceContext);

        mblas::Swap(State, PrevState);
        mblas::Swap(Embedding, PrevEmbedding);
        ++lengthIndex;
      }
      for (size_t j = 0; j < batch[0].size(); ++j) {
        std::cout
          << nbest[nbestIndex - sentences2score.size() + j][0] << " ||| "
          << nbest[nbestIndex - sentences2score.size() + j][1] << " ||| "
          << nbest[nbestIndex - sentences2score.size() + j][2] << " " 
          << fname << "= " << scores[j] << " ||| "
          << nbest[nbestIndex - sentences2score.size() + j][3] << std::endl;
      }
      index = boost::lexical_cast<size_t>(nbest[nbestIndex][0]);
    }
  }
  return 0;
}
