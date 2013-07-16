#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "Util.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
#include "TTableCollection.h"

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace Classifier;

class PhraseContextLine
{
public:
  PhraseContextLine(const string &line)
  {
    vector<string> columns = Tokenize(line, "\t");
    m_sentID   = Scan<size_t>(columns[0]);
    m_srcStart = Scan<size_t>(columns[1]);
    m_srcEnd   = Scan<size_t>(columns[2]);
    m_srcPhrase = columns[5];
    m_tgtPhrase = columns[6];
  }
  const string &GetSrcPhrase() { return m_srcPhrase; }
  const string &GetTgtPhrase() { return m_tgtPhrase; }
  size_t GetSentID()    { return m_sentID; }
  size_t GetSrcStart()  { return m_srcStart; }
  size_t GetSrcEnd()    { return m_srcEnd; }

private:
  PhraseContextLine();
  size_t m_sentID, m_srcStart, m_srcEnd;
  string m_srcPhrase, m_tgtPhrase;
};

ContextType ReadFactoredLine(const string &line, size_t factorCount)
{
  ContextType out;
  vector<string> words = Tokenize(line, " ");
  vector<string>::const_iterator it;
  for (it = words.begin(); it != words.end(); it++) {
    vector<string> factors = Tokenize(*it, "|");
    if (factors.size() != factorCount) {
      cerr << "error: Wrong count of factors: " << *it << endl;
      exit(1);
    }
    out.push_back(factors);
  }
  return out;
}

int main(int argc, char**argv)
{  
  if (argc < 6) {
    cerr << "error: wrong arguments" << endl;
    cerr << "Usage: extract-features context-file corpus phrase-tables extractor-config output-train" << endl;
    cerr << "  For multiple phrase tables (=domains), use id1:file1:::id2:file2" << endl;
    exit(1);
  }
  InputFileStream phraseContext(argv[1]);
  if (! phraseContext.good()) {
    cerr << "error: Failed to open " << argv[1] << endl;
    exit(1);
  }
  InputFileStream corpus(argv[2]);
  if (! corpus.good()) {
    cerr << "error: Failed to open " << argv[2] << endl;
    exit(1);
  }
  TTableCollection ttables(argv[3]);

  ExtractorConfig config;
  config.Load(argv[4]);
  FeatureExtractor extractor(config, true);
  VWFileTrainConsumer consumer(argv[5]);
  bool ttable_intersection = false;

  // parse options 
  // TODO use some library to do this
  for (int i = 6; i < argc; i++) {
    string opt = argv[i];
    if (opt == "--intersection") {
      ttable_intersection = true;
    } else {
      cerr << "Unknown option: " << opt << endl;
      exit(1);
    }
  }

  // one source phrase can have multiple correct translations
  // these will be on consecutive lines in the input context file
  string srcPhrase = "";
  ContextType context;
  vector<float> losses;
  vector<IndexedTranslation> translations;
  bool newSentence = false;
  size_t spanStart = 0;
  size_t spanEnd = 0;
  size_t sentID = 0;
  size_t srcTotal = 0;
  size_t tgtTotal = 0;
  size_t srcSurvived = 0;
  size_t tgtSurvived = 0;

  // don't generate features if no translations survived filtering
  bool hasTranslation = false;

  string corpusLine;
  string rawPhraseContextLine;
  while (getline(phraseContext, rawPhraseContextLine)) {
    tgtTotal++;
    PhraseContextLine contextLine(rawPhraseContextLine); // parse one line in context file

    // get to the current sentence in annotated corpus
    while (contextLine.GetSentID() > sentID) {
      getline(corpus, corpusLine);
      sentID++;
      newSentence = true;
    }

    if (! ttables.SrcExists(contextLine.GetSrcPhrase()))
      continue;

    if (contextLine.GetSrcPhrase() != srcPhrase || contextLine.GetSrcStart() != spanStart || newSentence) {
      // we have all correct translations of the current phrase, generate features
      if (hasTranslation) {
        srcSurvived++;
        extractor.GenerateFeatures(&consumer, context, spanStart, spanEnd,
            IndexedTranslation::Slice(translations), losses);
        newSentence = false;
      }
      
      // set new source phrase, context, translations and losses
      hasTranslation = false;
      srcPhrase = contextLine.GetSrcPhrase(); 
      spanStart = contextLine.GetSrcStart();
      spanEnd = contextLine.GetSrcEnd();
      context = ReadFactoredLine(corpusLine, config.GetFactors().size());
      translations = ttables.GetAllTranslations(srcPhrase, ttable_intersection);
      losses.clear();
      losses.resize(translations.size(), 1);
      srcTotal++;
    }

    bool foundTgt = false;
    size_t tgtPhraseID = ttables.GetTgtPhraseID(contextLine.GetTgtPhrase(), &foundTgt);
    
    if (foundTgt) {
      // add correct translation (i.e., set its loss to 0)
      for (size_t i = 0; i < translations.size(); i++) {
        if (translations[i].m_index == tgtPhraseID) {
          losses[i] = 0;
          hasTranslation = true;
          tgtSurvived++;
          break;
        }
      }
    } 
  }
  
  // generate features for the last source phrase
  if (hasTranslation) {
    srcSurvived++;
    extractor.GenerateFeatures(&consumer, context, spanStart, spanEnd,
        IndexedTranslation::Slice(translations), losses);
  }

  // output statistics about filtering
  cerr << "Filtered phrases: source " << srcTotal - srcSurvived << ", target " << tgtTotal - tgtSurvived << endl;
  cerr << "Remaining phrases: source " << srcSurvived << ", target " << tgtSurvived << endl;

  // flush FeatureConsumer
  consumer.Finish();
}
