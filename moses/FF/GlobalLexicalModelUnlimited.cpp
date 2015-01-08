#include "GlobalLexicalModelUnlimited.h"
#include <fstream>
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/Hypothesis.h"
#include "util/string_piece_hash.hh"

using namespace std;

namespace Moses
{
GlobalLexicalModelUnlimited::GlobalLexicalModelUnlimited(const std::string &line)
  :StatelessFeatureFunction(0, line)
{
  UTIL_THROW(util::Exception,
             "GlobalLexicalModelUnlimited hasn't been refactored for new feature function framework yet"); // TODO need to update arguments to key=value

  const vector<string> modelSpec = Tokenize(line);

  for (size_t i = 0; i < modelSpec.size(); i++ ) {
    bool ignorePunctuation = true, biasFeature = false, restricted = false;
    size_t context = 0;
    string filenameSource, filenameTarget;
    vector< string > factors;
    vector< string > spec = Tokenize(modelSpec[i]," ");

    // read optional punctuation and bias specifications
    if (spec.size() > 0) {
      if (spec.size() != 2 && spec.size() != 3 && spec.size() != 4 && spec.size() != 6) {
    	  std::cerr << "Format of glm feature is <factor-src>-<factor-tgt> [ignore-punct] [use-bias] "
                       <<  "[context-type] [filename-src filename-tgt]";
        //return false;
      }

      factors = Tokenize(spec[0],"-");
      if (spec.size() >= 2)
        ignorePunctuation = Scan<size_t>(spec[1]);
      if (spec.size() >= 3)
        biasFeature = Scan<size_t>(spec[2]);
      if (spec.size() >= 4)
        context = Scan<size_t>(spec[3]);
      if (spec.size() == 6) {
        filenameSource = spec[4];
        filenameTarget = spec[5];
        restricted = true;
      }
    } else
      factors = Tokenize(modelSpec[i],"-");

    if ( factors.size() != 2 ) {
    	std::cerr << "Wrong factor definition for global lexical model unlimited: " << modelSpec[i];
      //return false;
    }

    const vector<FactorType> inputFactors = Tokenize<FactorType>(factors[0],",");
    const vector<FactorType> outputFactors = Tokenize<FactorType>(factors[1],",");
    throw runtime_error("GlobalLexicalModelUnlimited should be reimplemented as a stateful feature");
    GlobalLexicalModelUnlimited* glmu = NULL; // new GlobalLexicalModelUnlimited(inputFactors, outputFactors, biasFeature, ignorePunctuation, context);

    if (restricted) {
      cerr << "loading word translation word lists from " << filenameSource << " and " << filenameTarget << endl;
      if (!glmu->Load(filenameSource, filenameTarget)) {
    	  std::cerr << "Unable to load word lists for word translation feature from files "
    			  << filenameSource
    			  << " and "
    			  << filenameTarget;
        //return false;
      }
    }
  }
}

bool GlobalLexicalModelUnlimited::Load(const std::string &filePathSource,
                                       const std::string &filePathTarget)
{
  // restricted source word vocabulary
  ifstream inFileSource(filePathSource.c_str());
  if (!inFileSource) {
    cerr << "could not open file " << filePathSource << endl;
    return false;
  }

  std::string line;
  while (getline(inFileSource, line)) {
    m_vocabSource.insert(line);
  }

  inFileSource.close();

  // restricted target word vocabulary
  ifstream inFileTarget(filePathTarget.c_str());
  if (!inFileTarget) {
    cerr << "could not open file " << filePathTarget << endl;
    return false;
  }

  while (getline(inFileTarget, line)) {
    m_vocabTarget.insert(line);
  }

  inFileTarget.close();

  m_unrestricted = false;
  return true;
}

void GlobalLexicalModelUnlimited::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

void GlobalLexicalModelUnlimited::EvaluateWhenApplied(const Hypothesis& cur_hypo, ScoreComponentCollection* accumulator) const
{
  const Sentence& input = *(m_local->input);
  const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();

  for(size_t targetIndex = 0; targetIndex < targetPhrase.GetSize(); targetIndex++ ) {
    StringPiece targetString = targetPhrase.GetWord(targetIndex).GetString(0); // TODO: change for other factors

    if (m_ignorePunctuation) {
      // check if first char is punctuation
      char firstChar = targetString[0];
      CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
    }

    if (m_biasFeature) {
      stringstream feature;
      feature << "glm_";
      feature << targetString;
      feature << "~";
      feature << "**BIAS**";
      accumulator->SparsePlusEquals(feature.str(), 1);
    }

    boost::unordered_set<uint64_t> alreadyScored;
    for(size_t sourceIndex = 0; sourceIndex < input.GetSize(); sourceIndex++ ) {
      const StringPiece sourceString = input.GetWord(sourceIndex).GetString(0);
      // TODO: change for other factors

      if (m_ignorePunctuation) {
        // check if first char is punctuation
        char firstChar = sourceString[0];
        CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
        if(charIterator != m_punctuationHash.end())
          continue;
      }
      const uint64_t sourceHash = util::MurmurHashNative(sourceString.data(), sourceString.size());

      if ( alreadyScored.find(sourceHash) == alreadyScored.end()) {
        bool sourceExists, targetExists;
        if (!m_unrestricted) {
          sourceExists = FindStringPiece(m_vocabSource, sourceString ) != m_vocabSource.end();
          targetExists = FindStringPiece(m_vocabTarget, targetString) != m_vocabTarget.end();
        }

        // no feature if vocab is in use and both words are not in restricted vocabularies
        if (m_unrestricted || (sourceExists && targetExists)) {
          if (m_sourceContext) {
            if (sourceIndex == 0) {
              // add <s> trigger feature for source
              stringstream feature;
              feature << "glm_";
              feature << targetString;
              feature << "~";
              feature << "<s>,";
              feature << sourceString;
              accumulator->SparsePlusEquals(feature.str(), 1);
              alreadyScored.insert(sourceHash);
            }

            // add source words to the right of current source word as context
            for(int contextIndex = sourceIndex+1; contextIndex < input.GetSize(); contextIndex++ ) {
              StringPiece contextString = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
              bool contextExists;
              if (!m_unrestricted)
                contextExists = FindStringPiece(m_vocabSource, contextString ) != m_vocabSource.end();

              if (m_unrestricted || contextExists) {
                stringstream feature;
                feature << "glm_";
                feature << targetString;
                feature << "~";
                feature << sourceString;
                feature << ",";
                feature << contextString;
                accumulator->SparsePlusEquals(feature.str(), 1);
                alreadyScored.insert(sourceHash);
              }
            }
          } else if (m_biphrase) {
            // --> look backwards for constructing context
            int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;

            // 1) source-target pair, trigger source word (can be discont.) and adjacent target word (bigram)
            StringPiece targetContext;
            if (globalTargetIndex > 0)
              targetContext = cur_hypo.GetWord(globalTargetIndex-1).GetString(0); // TODO: change for other factors
            else
              targetContext = "<s>";

            if (sourceIndex == 0) {
              StringPiece sourceTrigger = "<s>";
              AddFeature(accumulator, sourceTrigger, sourceString,
                         targetContext, targetString);
            } else
              for(int contextIndex = sourceIndex-1; contextIndex >= 0; contextIndex-- ) {
                StringPiece sourceTrigger = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
                bool sourceTriggerExists = false;
                if (!m_unrestricted)
                  sourceTriggerExists = FindStringPiece(m_vocabSource, sourceTrigger ) != m_vocabSource.end();

                if (m_unrestricted || sourceTriggerExists)
                  AddFeature(accumulator, sourceTrigger, sourceString,
                             targetContext, targetString);
              }

            // 2) source-target pair, adjacent source word (bigram) and trigger target word (can be discont.)
            StringPiece sourceContext;
            if (sourceIndex-1 >= 0)
              sourceContext = input.GetWord(sourceIndex-1).GetString(0); // TODO: change for other factors
            else
              sourceContext = "<s>";

            if (globalTargetIndex == 0) {
              string targetTrigger = "<s>";
              AddFeature(accumulator, sourceContext, sourceString,
                         targetTrigger, targetString);
            } else
              for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
                StringPiece targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
                bool targetTriggerExists = false;
                if (!m_unrestricted)
                  targetTriggerExists = FindStringPiece(m_vocabTarget, targetTrigger ) != m_vocabTarget.end();

                if (m_unrestricted || targetTriggerExists)
                  AddFeature(accumulator, sourceContext, sourceString,
                             targetTrigger, targetString);
              }
          } else if (m_bitrigger) {
            // allow additional discont. triggers on both sides
            int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;

            if (sourceIndex == 0) {
              StringPiece sourceTrigger = "<s>";
              bool sourceTriggerExists = true;

              if (globalTargetIndex == 0) {
                string targetTrigger = "<s>";
                bool targetTriggerExists = true;

                if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
                  AddFeature(accumulator, sourceTrigger, sourceString,
                             targetTrigger, targetString);
              } else {
                // iterate backwards over target
                for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
                  StringPiece targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
                  bool targetTriggerExists = false;
                  if (!m_unrestricted)
                    targetTriggerExists = FindStringPiece(m_vocabTarget, targetTrigger ) != m_vocabTarget.end();

                  if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
                    AddFeature(accumulator, sourceTrigger, sourceString,
                               targetTrigger, targetString);
                }
              }
            }
            // iterate over both source and target
            else {
              // iterate backwards over source
              for(int contextIndex = sourceIndex-1; contextIndex >= 0; contextIndex-- ) {
                StringPiece sourceTrigger = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
                bool sourceTriggerExists = false;
                if (!m_unrestricted)
                  sourceTriggerExists = FindStringPiece(m_vocabSource, sourceTrigger ) != m_vocabSource.end();

                if (globalTargetIndex == 0) {
                  string targetTrigger = "<s>";
                  bool targetTriggerExists = true;

                  if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
                    AddFeature(accumulator, sourceTrigger, sourceString,
                               targetTrigger, targetString);
                } else {
                  // iterate backwards over target
                  for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
                    StringPiece targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
                    bool targetTriggerExists = false;
                    if (!m_unrestricted)
                      targetTriggerExists = FindStringPiece(m_vocabTarget, targetTrigger ) != m_vocabTarget.end();

                    if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
                      AddFeature(accumulator, sourceTrigger, sourceString,
                                 targetTrigger, targetString);
                  }
                }
              }
            }
          } else {
            stringstream feature;
            feature << "glm_";
            feature << targetString;
            feature << "~";
            feature << sourceString;
            accumulator->SparsePlusEquals(feature.str(), 1);
            alreadyScored.insert(sourceHash);

          }
        }
      }
    }
  }
}

void GlobalLexicalModelUnlimited::AddFeature(ScoreComponentCollection* accumulator,
    StringPiece sourceTrigger, StringPiece sourceWord,
    StringPiece targetTrigger, StringPiece targetWord) const
{
  stringstream feature;
  feature << "glm_";
  feature << targetTrigger;
  feature << ",";
  feature << targetWord;
  feature << "~";
  feature << sourceTrigger;
  feature << ",";
  feature << sourceWord;
  accumulator->SparsePlusEquals(feature.str(), 1);

}

}
