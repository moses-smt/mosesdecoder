#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "Util.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
#include "RuleTable.h"
#include "InputTreeRep.h"

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace PSD;

class PSDLine
{
public:
  PSDLine(const string &line)
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
  PSDLine();
  size_t m_sentID, m_srcStart, m_srcEnd;
  string m_srcPhrase, m_tgtPhrase;
};

void WritePhraseIndex(const TargetIndexType &index, const string &outFile)
{
  OutputFileStream out(outFile);
  if (! out.good()) {
    cerr << "error: Failed to open " << outFile << endl;
    exit(1);
  }
  TargetIndexType::right_map::const_iterator it; // keys are sorted in the map
  for (it = index.right.begin(); it != index.right.end(); it++)
    out << it->second << "\n";
  out.Close();
}

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

size_t GetSizeOfSentence(const string &line)
{
	vector<string> words = Tokenize(line, " ");
	return words.size();
}

int main(int argc, char**argv)
{
  if (argc != 8) {
    cerr << "error: wrong arguments" << endl;
    cerr << "Usage: extract-psd psd-file parsed-file corpus phrase-table extractor-config output-train output-index" << endl;
    exit(1);
  }
  InputFileStream psd(argv[1]);
  if (! psd.good()) {
    cerr << "error: Failed to open " << argv[1] << endl;
    exit(1);
  }
  InputFileStream corpus(argv[2]);
  if (! corpus.good()) {
    cerr << "error: Failed to open " << argv[2] << endl;
    exit(1);
  }
  InputFileStream parse(argv[3]);
  if (! corpus.good()) {
    cerr << "error: Failed to open " << argv[3] << endl;
    exit(1);
  }

  RuleTable rtable(argv[4]);
  ExtractorConfig config;
  config.Load(argv[5]);
  FeatureExtractor extractor(rtable.GetTargetIndex(), config, true);
  VWFileTrainConsumer consumer(argv[6]);
  WritePhraseIndex(rtable.GetTargetIndex(), argv[7]);

  // one source phrase can have multiple correct translations
  // these will be on consecutive lines in the input PSD file
  string srcPhrase = "";
  ContextType context;
  vector<float> losses;
  vector<string> syntFeats;
  vector<Translation> translations;
  vector<SyntaxLabel> syntLabels;
  size_t spanStart = 0;
  size_t spanEnd = 0;
  size_t sentID = 0;
  size_t srcFiltered = 0;
  size_t tgtFiltered = 0;

  string corpusLine;
  string rawPSDLine;
  string parseLine;
  while (getline(psd, rawPSDLine)) {
    PSDLine psdLine(rawPSDLine); // parse one line in PSD file

    // get to the current sentence in annotated corpus
    while (psdLine.GetSentID() > sentID) {
      getline(corpus, corpusLine);
      getline(parse, parseLine);
      sentID++;
    }

    if (! rtable.SrcExists(psdLine.GetSrcPhrase())) {
      srcFiltered++;
      continue;
    }

    // we have all correct translations of the current phrase
    if (psdLine.GetSrcPhrase() != srcPhrase) {
      // generate features
      if (srcPhrase.length() != 0) // avoid the initial state
        extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, spanStart, spanEnd, translations, losses);

      // set new source phrase, context, translations and losses
      srcPhrase = psdLine.GetSrcPhrase();
      spanStart = psdLine.GetSrcStart();
      spanEnd = psdLine.GetSrcEnd();
      context = ReadFactoredLine(corpusLine, config.GetFactors().size());
      translations = rtable.GetTranslations(srcPhrase);
      losses.clear();
      losses.resize(translations.size(), 1);

     // set new syntax features
     size_t sentSize = GetSizeOfSentence(corpusLine);

    //cerr << "Extracting syntactic features..." << parseLineString << endl;
    Moses::InputTreeRep myInputChart = Moses::InputTreeRep(sentSize);
    myInputChart.Read(parseLine);
    //myInputChart.Print(std::cerr);

    //get syntax label associated to span
    vector<SyntaxLabel> syntaxLabels = myInputChart.GetLabels(spanStart, spanEnd);

    //iterate over labels and get strings
    //MAYBE INEFFICIENT
    vector<SyntaxLabel>::iterator itr_syn_lab;
    for(itr_syn_lab = syntaxLabels.begin(); itr_syn_lab != syntaxLabels.end(); itr_syn_lab++)
    {
        SyntaxLabel syntaxLabel = *itr_syn_lab;
        CHECK(syntaxLabel.IsNonTerm() == 1);
        string syntFeat = syntaxLabel.GetString();
        //std::cerr << "EXTRACTED FEATURE : " << syntFeat << std::endl;
        syntFeats.push_back(syntFeat);
    }
}
    bool foundTgt;
    size_t tgtPhraseID = rtable.GetTgtPhraseID(psdLine.GetTgtPhrase(), &foundTgt);
    if (foundTgt) {
      // add correct translation (i.e., set its loss to 0)
      for (size_t i = 0; i < translations.size(); i++) {
        if (translations[i].m_index == tgtPhraseID) {
          losses[i] = 0;
          break;
        }
      }
    } else {
      tgtFiltered++;
    }
  }

  // generate features for the last source phrase
  if (srcPhrase.length() != 0) // happens when source is empty
    extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, spanStart, spanEnd, translations, losses);

  // output statistics about filtering
  cerr << "Filtered phrases: source " << srcFiltered << ", target " << tgtFiltered << endl;

  // flush FeatureConsumer
  consumer.Finish();
}
