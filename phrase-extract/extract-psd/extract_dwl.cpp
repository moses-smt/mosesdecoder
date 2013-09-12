#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "Util.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "DWLFeatureExtractor.h"
#include "FeatureConsumer.h"
#include "TTableCollection.h"
#include "CeptTable.h"

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace PSD;

//FB : Pass a list of spans instead of a single span

class DWLLine
{
public:
  DWLLine(const string &line)
  {

    vector<string> columns = Tokenize(line, "\t");
    m_sentID = Scan<size_t>(columns[0]);

    //get and tokenize list of source/target spans
    //0-2,3-4	0-1
    m_sourceSpans = ReadSpanList(columns[1]);
    m_targetSpans = ReadSpanList(columns[2]);

    m_srcCept = columns[3];
    m_tgtCept = columns[4];
  }

  const string &GetSrcCept() { return m_srcCept; }
  const string &GetTgtCept() { return m_tgtCept; }
  size_t GetSentID()    { return m_sentID; }
  vector<pair<int, int> > GetSourceSpanList()  { return m_sourceSpans; }
  vector<pair<int, int> > GetTargetSpanList()  { return m_targetSpans; }

  bool IsSourceSorted()
  {
    int prevEnd = -1;
    vector<pair<int,int> >::const_iterator it;
    for (it = m_sourceSpans.begin(); it != m_sourceSpans.end(); it++) {
      if (it->first <= prevEnd) return false;
      prevEnd = it->second;
	  }
    return true;
  }

private:
  vector<pair<int, int> > ReadSpanList(const string &spanListStr)
  {
    vector<pair<int, int> > out;
    vector<string> spanList = Tokenize(spanListStr, " ");
    vector<string>::const_iterator spanIt;
    for (spanIt = spanList.begin(); spanIt != spanList.end(); spanIt++) {
      vector<string> positions = Tokenize(*spanIt, "-");      
      CHECK(positions.size() == 2);
      out.push_back(make_pair<int, int>(Scan<int>(positions[0]), Scan<int>(positions[1])));
    }
    CHECK(IsSourceSorted(out));
    return out;
  }

  DWLLine();
  size_t m_sentID;
  vector<pair<int,int> > m_sourceSpans;
  vector<pair<int,int> > m_targetSpans;
  vector<string> m_source, m_target;
  string m_srcCept, m_tgtCept;
};


void WritePhraseIndex(const IndexType *index, const string &outFile)
{
  OutputFileStream out(outFile);
  if (! out.good()) {
    cerr << "error: Failed to open " << outFile << endl;
    exit(1);
  }
  IndexType::right_map::const_iterator it; // keys are sorted in the map
  for (it = index->right.begin(); it != index->right.end(); it++)
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

int main(int argc, char**argv)
{
  if (argc < 7) {
    cerr << "error: wrong arguments" << endl;
    cerr << "Usage: extract-dwl dwl-file corpus phrase-tables extractor-config output-train output-index" << endl;
    cerr << "  For multiple phrase tables (=domains), use id1:file1:::id2:file2" << endl;
    exit(1);
  }
  InputFileStream dwl(argv[1]);
  if (! dwl.good()) {
    cerr << "error: Failed to open " << argv[1] << endl;
    exit(1);
  }
  InputFileStream corpus(argv[2]);
  if (! corpus.good()) {
    cerr << "error: Failed to open " << argv[2] << endl;
    exit(1);
  }

  CeptTable ctable(argv[3]);

  ExtractorConfig config;
  config.Load(argv[4]);
  DWLFeatureExtractor extractor(*ctable.GetTargetIndex(), config, true);
  VWFileTrainConsumer consumer(argv[5]);
  WritePhraseIndex(ctable.GetTargetIndex(), argv[6]);

  // one source phrase can have multiple correct translations
  // these will be on consecutive lines in the input PSD file
  string srcCept = "";
  ContextType context;
  vector<float> losses;
  vector<Translation> translations;
  bool newSentence = false;

  //FB : Instead of spanStart and spanEnd list of spans
  vector<pair<int,int> > sourceSpanList;
  size_t sentID = 0;
  size_t srcTotal = 0;
  size_t tgtTotal = 0;
  size_t srcSurvived = 0;
  size_t tgtSurvived = 0;

  // don't generate features if no translations survived filtering
  bool hasTranslation = false;

  string corpusLine;
  string rawDWLLine;
  while (getline(dwl, rawDWLLine)) {
    tgtTotal++;

    //FB : parse the extract.dwl file
    DWLLine dwlLine = DWLLine(rawDWLLine); // parse one line in PSD file

    // get to the current sentence in annotated corpus
    while (dwlLine.GetSentID() > sentID) {
      getline(corpus, corpusLine);
      sentID++;
      newSentence = true;
    }

    //FB : reimplement ctable into cepts
    if (! ctable.SrcExists(dwlLine.GetSrcCept()))
      continue;

    // we have all correct translations of the current phrase
    if (dwlLine.GetSrcCept() != srcCept || dwlLine.GetSourceSpanList() != sourceSpanList || newSentence) {
      // generate features
      if (hasTranslation) {
        srcSurvived++;
        extractor.GenerateFeatures(&consumer, context, sourceSpanList, translations, losses);
        newSentence = false;
      }

      // set new source phrase, context, translations and losses
      hasTranslation = false;
      srcCept = dwlLine.GetSrcCept();
      sourceSpanList = dwlLine.GetSourceSpanList();
      context = ReadFactoredLine(corpusLine, config.GetFactors().size());
      translations = ctable.GetTranslations(srcCept);
      losses.clear();
      losses.resize(translations.size(), 1);
      srcTotal++;
    }

    bool foundTgt = false;
    size_t tgtPhraseID = ctable.GetTgtPhraseID(dwlLine.GetTgtCept(), &foundTgt);

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
    extractor.GenerateFeatures(&consumer, context, sourceSpanList, translations, losses);
  }

  // output statistics about filtering
  cerr << "Filtered phrases: source " << srcTotal - srcSurvived << ", target " << tgtTotal - tgtSurvived << endl;
  cerr << "Remaining phrases: source " << srcSurvived << ", target " << tgtSurvived << endl;

  // flush FeatureConsumer
  consumer.Finish();
}
