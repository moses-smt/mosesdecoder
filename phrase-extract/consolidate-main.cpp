/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

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

#include <cstdlib>
#include <vector>
#include <string>

#include "util/exception.hh"
#include "moses/Util.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "PropertiesConsolidator.h"


bool countsProperty = false;
bool goodTuringFlag = false;
bool hierarchicalFlag = false;
bool kneserNeyFlag = false;
bool logProbFlag = false;
bool lowCountFlag = false;
bool onlyDirectFlag = false;
bool partsOfSpeechFlag = false;
bool phraseCountFlag = false;
bool sourceLabelsFlag = false;
bool targetSyntacticPreferencesFlag = false;
bool sparseCountBinFeatureFlag = false;

std::vector< int > countBin;
float minScore0 = 0;
float minScore2 = 0;

std::vector< float > countOfCounts;
std::vector< float > goodTuringDiscount;
float kneserNey_D1, kneserNey_D2, kneserNey_D3, totalCount = -1;


void processFiles( const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, const std::string& );
void loadCountOfCounts( const std::string& );
void breakdownCoreAndSparse( const std::string &combined, std::string &core, std::string &sparse );
bool getLine( Moses::InputFileStream &file, std::vector< std::string > &item );


inline float maybeLogProb( float a )
{
  return logProbFlag ? std::log(a) : a;
}


inline bool isNonTerminal( const std::string &word )
{
  return (word.length()>=3 && word[0] == '[' && word[word.length()-1] == ']');
}


int main(int argc, char* argv[])
{
  std::cerr << "Consolidate v2.0 written by Philipp Koehn" << std::endl
            << "consolidating direct and indirect rule tables" << std::endl;

  if (argc < 4) {
    std::cerr <<
              "syntax: "
              "consolidate phrase-table.direct "
              "phrase-table.indirect "
              "phrase-table.consolidated "
              "[--Hierarchical] [--OnlyDirect] [--PhraseCount] "
              "[--GoodTuring counts-of-counts-file] "
              "[--KneserNey counts-of-counts-file] [--LowCountFeature] "
              "[--SourceLabels source-labels-file] "
              "[--PartsOfSpeech parts-of-speech-file] "
              "[--MinScore id:threshold[,id:threshold]*]"
              << std::endl;
    exit(1);
  }
  const std::string fileNameDirect = argv[1];
  const std::string fileNameIndirect = argv[2];
  const std::string fileNameConsolidated = argv[3];
  std::string fileNameCountOfCounts;
  std::string fileNameSourceLabelSet;
  std::string fileNamePartsOfSpeechVocabulary;
  std::string fileNameTargetSyntacticPreferencesLabelSet;

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      std::cerr << "processing hierarchical rules" << std::endl;
    } else if (strcmp(argv[i],"--OnlyDirect") == 0) {
      onlyDirectFlag = true;
      std::cerr << "only including direct translation scores p(e|f)" << std::endl;
    } else if (strcmp(argv[i],"--PhraseCount") == 0) {
      phraseCountFlag = true;
      std::cerr << "including the phrase count feature" << std::endl;
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      UTIL_THROW_IF2(i+1==argc, "specify count of count files for Good Turing discounting!");
      fileNameCountOfCounts = argv[++i];
      std::cerr << "adjusting phrase translation probabilities with Good Turing discounting" << std::endl;
    } else if (strcmp(argv[i],"--KneserNey") == 0) {
      kneserNeyFlag = true;
      UTIL_THROW_IF2(i+1==argc, "specify count of count files for Kneser Ney discounting!");
      fileNameCountOfCounts = argv[++i];
      std::cerr << "adjusting phrase translation probabilities with Kneser Ney discounting" << std::endl;
    } else if (strcmp(argv[i],"--LowCountFeature") == 0) {
      lowCountFlag = true;
      std::cerr << "including the low count feature" << std::endl;
    } else if (strcmp(argv[i],"--CountBinFeature") == 0 ||
               strcmp(argv[i],"--SparseCountBinFeature") == 0) {
      if (strcmp(argv[i],"--SparseCountBinFeature") == 0)
        sparseCountBinFeatureFlag = true;
      std::cerr << "include "<< (sparseCountBinFeatureFlag ? "sparse " : "") << "count bin feature:";
      int prev = 0;
      while(i+1<argc && argv[i+1][0]>='0' && argv[i+1][0]<='9') {
        int binCount = std::atoi( argv[++i] );
        countBin.push_back( binCount );
        if (prev+1 == binCount) {
          std::cerr << " " << binCount;
        } else {
          std::cerr << " " << (prev+1) << "-" << binCount;
        }
        prev = binCount;
      }
      std::cerr << " " << (prev+1) << "+" << std::endl;
    } else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      std::cerr << "using log-probabilities" << std::endl;
    } else if (strcmp(argv[i],"--Counts") == 0) {
      countsProperty = true;
      std::cerr << "output counts as a property" << std::endl;;
    } else if (strcmp(argv[i],"--SourceLabels") == 0) {
      sourceLabelsFlag = true;
      UTIL_THROW_IF2(i+1==argc, "specify source label set file!");
      fileNameSourceLabelSet = argv[++i];
      std::cerr << "processing source labels property" << std::endl;
    } else if (strcmp(argv[i],"--PartsOfSpeech") == 0) {
      partsOfSpeechFlag = true;
      UTIL_THROW_IF2(i+1==argc, "specify parts-of-speech file!");
      fileNamePartsOfSpeechVocabulary = argv[++i];
      std::cerr << "processing parts-of-speech property" << std::endl;
    } else if (strcmp(argv[i],"--TargetSyntacticPreferences") == 0) {
      targetSyntacticPreferencesFlag = true;
      UTIL_THROW_IF2(i+1==argc, "specify target syntactic preferences label set file!");
      fileNameTargetSyntacticPreferencesLabelSet = argv[++i];
      std::cerr << "processing target syntactic preferences property" << std::endl;
    } else if (strcmp(argv[i],"--MinScore") == 0) {
      std::string setting = argv[++i];
      bool done = false;
      while (!done) {
        std::string single_setting;
        size_t pos;
        if ((pos = setting.find(",")) != std::string::npos) {
          single_setting = setting.substr(0, pos);
          setting.erase(0, pos + 1);
        } else {
          single_setting = setting;
          done = true;
        }
        pos = single_setting.find(":");
        UTIL_THROW_IF2(pos == std::string::npos, "faulty MinScore setting '" << single_setting << "' in '" << argv[i] << "'");
        unsigned int field = atoll( single_setting.substr(0,pos).c_str() );
        float threshold = std::atof( single_setting.substr(pos+1).c_str() );
        if (field == 0) {
          minScore0 = threshold;
          std::cerr << "setting minScore0 to " << threshold << std::endl;
        } else if (field == 2) {
          minScore2 = threshold;
          std::cerr << "setting minScore2 to " << threshold << std::endl;
        } else {
          UTIL_THROW2("MinScore currently only supported for indirect (0) and direct (2) phrase translation probabilities");
        }
      }
    } else {
      UTIL_THROW2("unknown option " << argv[i]);
    }
  }

  processFiles( fileNameDirect, fileNameIndirect, fileNameConsolidated, fileNameCountOfCounts, fileNameSourceLabelSet, fileNamePartsOfSpeechVocabulary, fileNameTargetSyntacticPreferencesLabelSet );
}


void loadCountOfCounts( const std::string& fileNameCountOfCounts )
{
  Moses::InputFileStream fileCountOfCounts(fileNameCountOfCounts);
  UTIL_THROW_IF2(fileCountOfCounts.fail(), "could not open count of counts file " << fileNameCountOfCounts);

  countOfCounts.push_back(0.0);

  std::string line;
  while (getline(fileCountOfCounts, line)) {
    if (totalCount < 0)
      totalCount = std::atof( line.c_str() ); // total number of distinct phrase pairs
    else
      countOfCounts.push_back( std::atof( line.c_str() ) );
  }
  fileCountOfCounts.Close();

  // compute Good Turing discounts
  if (goodTuringFlag) {
    goodTuringDiscount.push_back(0.01); // floor value
    for( size_t i=1; i<countOfCounts.size()-1; i++ ) {
      goodTuringDiscount.push_back(((float)i+1)/(float)i*((countOfCounts[i+1]+0.1) / ((float)countOfCounts[i]+0.1)));
      if (goodTuringDiscount[i]>1)
        goodTuringDiscount[i] = 1;
      if (goodTuringDiscount[i]<goodTuringDiscount[i-1])
        goodTuringDiscount[i] = goodTuringDiscount[i-1];
    }
  }

  // compute Kneser Ney co-efficients [Chen&Goodman, 1998]
  float Y = countOfCounts[1] / (countOfCounts[1] + 2*countOfCounts[2]);
  kneserNey_D1 = 1 - 2*Y * countOfCounts[2] / countOfCounts[1];
  kneserNey_D2 = 2 - 3*Y * countOfCounts[3] / countOfCounts[2];
  kneserNey_D3 = 3 - 4*Y * countOfCounts[4] / countOfCounts[3];
  // sanity constraints
  if (kneserNey_D1 > 0.9) kneserNey_D1 = 0.9;
  if (kneserNey_D2 > 1.9) kneserNey_D2 = 1.9;
  if (kneserNey_D3 > 2.9) kneserNey_D3 = 2.9;
}


void processFiles( const std::string& fileNameDirect,
                   const std::string& fileNameIndirect,
                   const std::string& fileNameConsolidated,
                   const std::string& fileNameCountOfCounts,
                   const std::string& fileNameSourceLabelSet,
                   const std::string& fileNamePartsOfSpeechVocabulary,
                   const std::string& fileNameTargetSyntacticPreferencesLabelSet )
{
  if (goodTuringFlag || kneserNeyFlag)
    loadCountOfCounts( fileNameCountOfCounts );

  // open input files
  Moses::InputFileStream fileDirect(fileNameDirect);
  UTIL_THROW_IF2(fileDirect.fail(), "could not open phrase table file " << fileNameDirect);
  Moses::InputFileStream fileIndirect(fileNameIndirect);
  UTIL_THROW_IF2(fileIndirect.fail(), "could not open phrase table file " << fileNameIndirect);

  // open output file: consolidated phrase table
  Moses::OutputFileStream fileConsolidated;
  bool success = fileConsolidated.Open(fileNameConsolidated);
  UTIL_THROW_IF2(!success, "could not open output file " << fileNameConsolidated);

  // create properties consolidator
  // (in case any additional phrase property requires further processing)
  MosesTraining::PropertiesConsolidator propertiesConsolidator = MosesTraining::PropertiesConsolidator();
  if (sourceLabelsFlag) {
    propertiesConsolidator.ActivateSourceLabelsProcessing(fileNameSourceLabelSet);
  }
  if (partsOfSpeechFlag) {
    propertiesConsolidator.ActivatePartsOfSpeechProcessing(fileNamePartsOfSpeechVocabulary);
  }
  if (targetSyntacticPreferencesFlag) {
    propertiesConsolidator.ActivateTargetSyntacticPreferencesProcessing(fileNameTargetSyntacticPreferencesLabelSet);
  }

  // loop through all extracted phrase translations
  int i=0;
  while(true) {
    // Print progress dots to stderr.
    i++;
    if (i%100000 == 0) std::cerr << "." << std::flush;

    std::vector< std::string > itemDirect, itemIndirect;
    if (! getLine(fileIndirect, itemIndirect) ||
        ! getLine(fileDirect, itemDirect))
      break;

    // direct: target source alignment probabilities
    // indirect: source target probabilities

    // consistency checks
    UTIL_THROW_IF2(itemDirect[0].compare( itemIndirect[0] ) != 0,
                   "target phrase does not match in line " << i << ": '" << itemDirect[0] << "' != '" << itemIndirect[0] << "'");
    UTIL_THROW_IF2(itemDirect[1].compare( itemIndirect[1] ) != 0,
                   "source phrase does not match in line " << i << ": '" << itemDirect[1] << "' != '" << itemIndirect[1] << "'");

    // SCORES ...
    std::string directScores, directSparseScores, indirectScores, indirectSparseScores;
    breakdownCoreAndSparse( itemDirect[3], directScores, directSparseScores );
    breakdownCoreAndSparse( itemIndirect[3], indirectScores, indirectSparseScores );

    std::vector<std::string> directCounts;
    Moses::Tokenize( directCounts, itemDirect[4] );
    std::vector<std::string> indirectCounts;
    Moses::Tokenize( indirectCounts, itemIndirect[4] );
    float countF  = std::atof( directCounts[0].c_str() );
    float countE  = std::atof( indirectCounts[0].c_str() );
    float countEF = std::atof( indirectCounts[1].c_str() );
    float n1_F, n1_E;
    if (kneserNeyFlag) {
      n1_F = std::atof( directCounts[2].c_str() );
      n1_E = std::atof( indirectCounts[2].c_str() );
    }

    // Good Turing discounting
    float adjustedCountEF = countEF;
    if (goodTuringFlag && countEF+0.99999 < goodTuringDiscount.size()-1)
      adjustedCountEF *= goodTuringDiscount[(int)(countEF+0.99998)];
    float adjustedCountEF_indirect = adjustedCountEF;

    // Kneser Ney discounting [Foster et al, 2006]
    if (kneserNeyFlag) {
      float D = kneserNey_D3;
      if (countEF < 2) D = kneserNey_D1;
      else if (countEF < 3) D = kneserNey_D2;
      if (D > countEF) D = countEF - 0.01; // sanity constraint

      float p_b_E = n1_E / totalCount; // target phrase prob based on distinct
      float alpha_F = D * n1_F / countF; // available mass
      adjustedCountEF = countEF - D + countF * alpha_F * p_b_E;

      // for indirect
      float p_b_F = n1_F / totalCount; // target phrase prob based on distinct
      float alpha_E = D * n1_E / countE; // available mass
      adjustedCountEF_indirect = countEF - D + countE * alpha_E * p_b_F;
    }

    // drop due to MinScore thresholding
    if ((minScore0 > 0 && adjustedCountEF_indirect/countE < minScore0) ||
        (minScore2 > 0 && adjustedCountEF         /countF < minScore2)) {
      continue;
    }

    // output phrase pair
    fileConsolidated << itemDirect[0] << " ||| ";

    if (partsOfSpeechFlag) {
      // write POS factor from property
      std::vector<std::string> targetTokens;
      Moses::Tokenize( targetTokens, itemDirect[1] );
      std::vector<std::string> propertyValuePOS;
      propertiesConsolidator.GetPOSPropertyValueFromPropertiesString(itemDirect[5], propertyValuePOS);
      size_t targetTerminalIndex = 0;
      for (std::vector<std::string>::const_iterator targetTokensIt=targetTokens.begin();
           targetTokensIt!=targetTokens.end(); ++targetTokensIt) {
        fileConsolidated << *targetTokensIt;
        if (!isNonTerminal(*targetTokensIt)) {
          assert(propertyValuePOS.size() > targetTerminalIndex);
          fileConsolidated << "|" << propertyValuePOS[targetTerminalIndex];
          ++targetTerminalIndex;
        }
        fileConsolidated << " ";
      }
      fileConsolidated << "|||";

    } else {

      fileConsolidated << itemDirect[1] << " |||";
    }


    // prob indirect
    if (!onlyDirectFlag) {
      fileConsolidated << " " << maybeLogProb(adjustedCountEF_indirect/countE);
      fileConsolidated << " " << indirectScores;
    }

    // prob direct
    fileConsolidated << " " << maybeLogProb(adjustedCountEF/countF);
    fileConsolidated << " " << directScores;

    // phrase count feature
    if (phraseCountFlag) {
      fileConsolidated << " " << maybeLogProb(2.718);
    }

    // low count feature
    if (lowCountFlag) {
      fileConsolidated << " " << maybeLogProb(std::exp(-1.0/countEF));
    }

    // count bin feature (as a core feature)
    if (countBin.size()>0 && !sparseCountBinFeatureFlag) {
      bool foundBin = false;
      for(size_t i=0; i < countBin.size(); i++) {
        if (!foundBin && countEF <= countBin[i]) {
          fileConsolidated << " " << maybeLogProb(2.718);
          foundBin = true;
        } else {
          fileConsolidated << " " << maybeLogProb(1);
        }
      }
      fileConsolidated << " " << maybeLogProb( foundBin ? 1 : 2.718 );
    }

    // alignment
    fileConsolidated << " |||";
    if (!itemDirect[2].empty()) {
      fileConsolidated << " " << itemDirect[2];;
    }

    // counts, for debugging
    fileConsolidated << " ||| " << countE << " " << countF << " " << countEF;

    // sparse features
    fileConsolidated << " |||";
    if (directSparseScores.compare("") != 0)
      fileConsolidated << " " << directSparseScores;
    if (indirectSparseScores.compare("") != 0)
      fileConsolidated << " " << indirectSparseScores;

    // count bin feature (as a sparse feature)
    if (sparseCountBinFeatureFlag) {
      bool foundBin = false;
      for(size_t i=0; i < countBin.size(); i++) {
        if (!foundBin && countEF <= countBin[i]) {
          fileConsolidated << " cb_";
          if (i == 0 && countBin[i] > 1)
            fileConsolidated << "1_";
          else if (i > 0 && countBin[i-1]+1 < countBin[i])
            fileConsolidated << (countBin[i-1]+1) << "_";
          fileConsolidated << countBin[i] << " 1";
          foundBin = true;
        }
      }
      if (!foundBin) {
        fileConsolidated << " cb_max 1";
      }
    }

    // arbitrary key-value pairs
    fileConsolidated << " |||";
    if (itemDirect.size() >= 6) {
      propertiesConsolidator.ProcessPropertiesString(itemDirect[5], fileConsolidated);
    }

    if (countsProperty) {
      fileConsolidated << " {{Counts " << countE << " " << countF << " " << countEF << "}}";
    }

    fileConsolidated << std::endl;
  }

  fileDirect.Close();
  fileIndirect.Close();
  fileConsolidated.Close();

  // We've been printing progress dots to stderr.  End the line.
  std::cerr << std::endl;
}


void breakdownCoreAndSparse( const std::string &combined, std::string &core, std::string &sparse )
{
  core = "";
  sparse = "";
  std::vector<std::string> score;
  Moses::Tokenize( score, combined );
  for(size_t i=0; i<score.size(); i++) {
    if ((score[i][0] >= '0' && score[i][0] <= '9') || i+1 == score.size())
      core += " " + score[i];
    else {
      sparse += " " + score[i];
      sparse += " " + score[++i];
    }
  }
  if (core.size() > 0 ) core = core.substr(1);
  if (sparse.size() > 0 ) sparse = sparse.substr(1);
}


bool getLine( Moses::InputFileStream &file, std::vector< std::string > &item )
{
  if (file.eof())
    return false;

  std::string line;
  if (!getline(file, line))
    return false;

  Moses::TokenizeMultiCharSeparator(item, line, " ||| ");

  return true;
}

