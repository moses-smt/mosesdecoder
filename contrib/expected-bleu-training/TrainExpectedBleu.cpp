/*
   Moses - statistical machine translation system
   Copyright (C) 2005-2015 University of Edinburgh

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
*/


#include "ExpectedBleuOptimizer.h"
#include "util/file_stream.hh"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include <sstream>
#include <boost/program_options.hpp>

using namespace ExpectedBleuTraining;
namespace po = boost::program_options;


int main(int argc, char **argv) {

  util::FileStream out(1);
  util::FileStream err(2);

  size_t maxNBestSize;
  size_t iterationLimit;
  std::string filenameSBleu, filenameNBestList, filenameFeatureNames, filenameInitialWeights;

  bool ignoreDecoderScore;

  float learningRate;
  float initialStepSize;
  float decreaseRate;
  float increaseRate;
  float minStepSize;
  float maxStepSize;
  float floorAbsScalingFactor;
  float regularizationParameter;
  bool printZeroWeights;
  bool miniBatches;
  std::string optimizerTypeStr;
  size_t optimizerType = 0;
#define EXPECTED_BLEU_OPTIMIZER_TYPE_RPROP 1
#define EXPECTED_BLEU_OPTIMIZER_TYPE_SGD 2

  try {

    po::options_description descr("Usage");
    descr.add_options()
      ("help,h", "produce help message")
      ("n-best-size-limit,l", po::value<size_t>(&maxNBestSize)->default_value(100), 
       "limit of n-best list entries to be considered for training")
      ("iterations,i", po::value<size_t>(&iterationLimit)->default_value(50), 
       "number of training iterations")
      ("sbleu-file,b", po::value<std::string>(&filenameSBleu)->required(), 
       "file containing sentence-level BLEU scores for all n-best list entries")
      ("prepared-n-best-list,n", po::value<std::string>(&filenameNBestList)->required(), 
       "input n-best list file, in prepared format for expected BLEU training")
      ("feature-name-file,f", po::value<std::string>(&filenameFeatureNames)->required(), 
       "file containing mapping between feature names and indices")
      ("initial-weights-file,w", po::value<std::string>(&filenameInitialWeights)->default_value(""), 
       "file containing start values for scaling factors (optional)")
      ("ignore-decoder-score", boost::program_options::value<bool>(&ignoreDecoderScore)->default_value(0), 
       "exclude decoder score from computation of posterior probability")
      ("regularization", boost::program_options::value<float>(&regularizationParameter)->default_value(0), // e.g. 1e-5 
       "regularization parameter; suggested value range: [1e-8,1e-5]")
      ("learning-rate", boost::program_options::value<float>(&learningRate)->default_value(1), 
       "learning rate for the SGD optimizer")
      ("floor", boost::program_options::value<float>(&floorAbsScalingFactor)->default_value(0),  // e.g. 1e-7
       "set scaling factor to 0 if below this absolute value after update")
      ("initial-step-size", boost::program_options::value<float>(&initialStepSize)->default_value(0.001),  // TODO: try 0.01 and 0.1
       "initial step size for the RPROP optimizer")
      ("decrease-rate", boost::program_options::value<float>(&decreaseRate)->default_value(0.5), 
       "decrease rate for the RPROP optimizer")
      ("increase-rate", boost::program_options::value<float>(&increaseRate)->default_value(1.2), 
       "increase rate for the RPROP optimizer")
      ("min-step-size", boost::program_options::value<float>(&minStepSize)->default_value(1e-7), 
       "minimum step size for the RPROP optimizer")
      ("max-step-size", boost::program_options::value<float>(&maxStepSize)->default_value(1), 
       "maximum step size for the RPROP optimizer")
      ("print-zero-weights", boost::program_options::value<bool>(&printZeroWeights)->default_value(0), 
       "output scaling factors even if they are trained to 0")
      ("optimizer", po::value<std::string>(&optimizerTypeStr)->default_value("RPROP"), 
       "optimizer type used for training (known algorithms: RPROP, SGD)")
      ("mini-batches", boost::program_options::value<bool>(&miniBatches)->default_value(0), 
       "update after every single sentence (SGD only)")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, descr), vm);

    if (vm.count("help")) {
      std::ostringstream os;
      os << descr;
      out << os.str() << '\n';
      out.flush();
      exit(0);
    }

    po::notify(vm);

  } catch(std::exception& e) {

    err << "Error: " << e.what() << '\n';
    err.flush();
    exit(1);
  }

  if ( !optimizerTypeStr.compare("rprop") || !optimizerTypeStr.compare("RPROP") ) {
    optimizerType = EXPECTED_BLEU_OPTIMIZER_TYPE_RPROP;
  } else if ( !optimizerTypeStr.compare("sgd") || !optimizerTypeStr.compare("SGD") ) {
    optimizerType = EXPECTED_BLEU_OPTIMIZER_TYPE_SGD;
  } else {
    err << "Error: unknown optimizer type: \"" << optimizerTypeStr << "\" (known optimizers: rprop, sgd) " << '\n';
    err.flush();
    exit(1);
  }

  

  util::FilePiece ifsFeatureNames(filenameFeatureNames.c_str());

  StringPiece lineFeatureName;
  if ( !ifsFeatureNames.ReadLineOrEOF(lineFeatureName) ) 
  {
    err << "Error: flawed content in " << filenameFeatureNames << '\n';
    err.flush();
    exit(1);
  }
  size_t maxFeatureNamesIdx = atol( lineFeatureName.as_string().c_str() );

  std::vector<std::string> featureNames(maxFeatureNamesIdx);
  boost::unordered_map<std::string, size_t> featureIndexes;
  for (size_t i=0; i<maxFeatureNamesIdx; ++i) 
  {
    if ( !ifsFeatureNames.ReadLineOrEOF(lineFeatureName) ) {
      err << "Error: flawed content in " << filenameFeatureNames << '\n';
      err.flush();
      exit(1);
    }
    util::TokenIter<util::SingleCharacter> token(lineFeatureName, ' ');
    size_t featureIndexCurrent = atol( token->as_string().c_str() );
    token++;
    featureNames[featureIndexCurrent] = token->as_string();
    featureIndexes[token->as_string()] = featureIndexCurrent;
  }


  std::vector<float> sparseScalingFactor(maxFeatureNamesIdx);
  std::vector< boost::unordered_map<size_t, float> > sparseScore(maxNBestSize);

  // read initial weights, if any given

  if ( filenameInitialWeights.length() != 0 ) 
  {
    util::FilePiece ifsInitialWeights(filenameInitialWeights.c_str());

    StringPiece lineInitialWeight;
    if ( !ifsInitialWeights.ReadLineOrEOF(lineInitialWeight) ) {
      err << "Error: flawed content in " << filenameInitialWeights << '\n';
      err.flush();
      exit(1);
    }
    do {
      util::TokenIter<util::SingleCharacter> token(lineInitialWeight, ' ');
      boost::unordered_map<std::string, size_t>::const_iterator found = featureIndexes.find(token->as_string());
      if ( found == featureIndexes.end() ) {
        err << "Error: flawed content in " << filenameInitialWeights << " (unkown feature name \"" << token->as_string() << "\")" << '\n';
        err.flush();
        exit(1);
      }
      token++;
      sparseScalingFactor[found->second] = atof( token->as_string().c_str() );
    } while ( ifsInitialWeights.ReadLineOrEOF(lineInitialWeight) );
  }

  // train

  ExpectedBleuOptimizer optimizer(err, 
                                  learningRate, 
                                  initialStepSize, 
                                  decreaseRate, 
                                  increaseRate,
                                  minStepSize, 
                                  maxStepSize, 
                                  floorAbsScalingFactor, 
                                  regularizationParameter);

  if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_RPROP ) 
  {
    optimizer.InitRPROP(sparseScalingFactor);
  } else if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_SGD ) {
    optimizer.InitRPROP(sparseScalingFactor);
  } else {
    err << "Error: unknown optimizer type" << '\n';
    err.flush();
    exit(1);
  }
  
  for (size_t nIteration=1; nIteration<=iterationLimit; ++nIteration) 
  {
    util::FilePiece ifsSBleu(filenameSBleu.c_str());
    util::FilePiece ifsNBest(filenameNBestList.c_str());

    out << "### ITERATION " << nIteration << '\n' << '\n';

    size_t sentenceIndex = 0;
    size_t batchSize = 0;
    size_t nBestSizeCount = 0;
    size_t globalIndex = 0;
    StringPiece lineNBest;
    std::vector<double> overallScoreUntransformed;
    std::vector<float> sBleu;
    float xBleu = 0;
    // double expPrecisionCorrection = 0.0;

    while ( ifsNBest.ReadLineOrEOF(lineNBest) )
    {

      util::TokenIter<util::SingleCharacter> token(lineNBest, ' ');

      if ( token == token.end() )
      {
        err << "Error: flawed content in " << filenameNBestList << '\n';
        err.flush();
        exit(1);
      }

      size_t sentenceIndexCurrent = atol( token->as_string().c_str() );
      token++;

      if ( sentenceIndex != sentenceIndexCurrent )
      {

        if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_RPROP ) 
        {
          optimizer.AddTrainingInstance( nBestSizeCount, sBleu, overallScoreUntransformed, sparseScore );
        } else if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_SGD ) {
          optimizer.AddTrainingInstance( nBestSizeCount, sBleu, overallScoreUntransformed, sparseScore, miniBatches );

          if ( miniBatches ) {
            xBleu += optimizer.UpdateSGD( sparseScalingFactor, 1 );
            // out << "ITERATION " << nIteration << " SENTENCE " << sentenceIndex << " XBLEUSUM= " << xBleu << '\n';
            // for (size_t i=0; i<sparseScalingFactor.size(); ++i)
            // {
            //   if ( (sparseScalingFactor[i] != 0) || printZeroWeights )
            //   {
            //     out << "ITERATION " << nIteration << " WEIGHT " << featureNames[i] << " " << sparseScalingFactor[i] << '\n';
            //   }
            // }
            // out << '\n';
            // out.flush();
          }
        } else {
           err << "Error: unknown optimizer type" << '\n';
           err.flush();
           exit(1);
        }

        for (size_t i=0; i<nBestSizeCount; ++i) {
          sparseScore[i].clear();
        }
        nBestSizeCount = 0;
        overallScoreUntransformed.clear();
        sBleu.clear();
        sentenceIndex = sentenceIndexCurrent;
        ++batchSize;
      }

      StringPiece lineSBleu;
      if ( !ifsSBleu.ReadLineOrEOF(lineSBleu) )
      {
        err << "Error: insufficient number of lines in " << filenameSBleu << '\n';
        err.flush();
        exit(1);
      }

      if ( nBestSizeCount < maxNBestSize )
      {
        // retrieve sBLEU

        float sBleuCurrent = atof( lineSBleu.as_string().c_str() );
        sBleu.push_back(sBleuCurrent);

        // process n-best list entry

        if ( token == token.end() )
        {
          err << "Error: flawed content in " << filenameNBestList << '\n';
          err.flush();
          exit(1);
        }
        double scoreCurrent = 0; 
        if ( !ignoreDecoderScore ) 
        {
          scoreCurrent = atof( token->as_string().c_str() ); // decoder score 
        }
        token++;

        // if ( nBestSizeCount == 0 ) // best translation (first n-best list entry for the current sentence / a new mini-batch)
        // {
        //   expPrecisionCorrection = std::floor ( scoreCurrent ); // decoder score of first-best
        // }

        while (token != token.end())
        {
          size_t featureNameCurrent = atol( token->as_string().c_str() );
          token++;
          float featureValueCurrent = atof( token->as_string().c_str() );
          sparseScore[nBestSizeCount].insert(std::make_pair(featureNameCurrent, featureValueCurrent));
          scoreCurrent += sparseScalingFactor[featureNameCurrent] * featureValueCurrent;
          token++;
        }

        // overallScoreUntransformed.push_back( std::exp(scoreCurrent - expPrecisionCorrection) );
        overallScoreUntransformed.push_back( std::exp(scoreCurrent) );

        ++nBestSizeCount;
      }
      ++globalIndex;
    }

    if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_RPROP ) 
    {
      optimizer.AddTrainingInstance( nBestSizeCount, sBleu, overallScoreUntransformed, sparseScore ); // last sentence in the corpus
      xBleu = optimizer.UpdateRPROP( sparseScalingFactor, batchSize );
      out << "xBLEU= " << xBleu << '\n';
    } else if ( optimizerType == EXPECTED_BLEU_OPTIMIZER_TYPE_SGD ) {
      optimizer.AddTrainingInstance( nBestSizeCount, sBleu, overallScoreUntransformed, sparseScore, miniBatches ); // last sentence in the corpus
      if ( miniBatches ) {
        xBleu += optimizer.UpdateSGD( sparseScalingFactor, 1 );
        xBleu /= batchSize;
      } else {
        xBleu = optimizer.UpdateSGD( sparseScalingFactor, batchSize );
      }
      out << "xBLEU= " << xBleu << '\n';
    } else {
      err << "Error: unknown optimizer type" << '\n';
      err.flush();
      exit(1);
    }

    for (size_t i=0; i<nBestSizeCount; ++i) {
      sparseScore[i].clear();
    }
    nBestSizeCount = 0;
    overallScoreUntransformed.clear();
    sBleu.clear();

    out << '\n';

    for (size_t i=0; i<sparseScalingFactor.size(); ++i)
    {
      if ( (sparseScalingFactor[i] != 0) || printZeroWeights )
      {
        out << "ITERATION " << nIteration << " WEIGHT " << featureNames[i] << " " << sparseScalingFactor[i] << '\n';
      }
    }

    out << '\n';
    out.flush();
  }

}

