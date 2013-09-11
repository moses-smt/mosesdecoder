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
using namespace PSD;

//FB : Pass a list of spans instead of a single span

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
  string m_srcCept, m_tgtCept;
};

class DWLLine
{
public:
  DWLLine(const string &line)
  {

    vector<string> columns = Tokenize(line, "\t");
    m_sentID   = Scan<size_t>(columns[0]);

    //get and tokenize list of source/target spans
    //0-2,3-4	0-1
    vector<string> StartSpanList = Tokenize(columns[1],",");
    vector<string> EndSpanList =  Tokenize(columns[2],",");

    for(int i = StartSpanList.begin(); i != StartSpanList.end(); i++)
    {
    	vector<string> currentSpan= Tokenize(*i,"-");

    	//each pair should contain exactly 2 elements
    	CHECK(currentSpan.size() == 2);
    	size_t firstSpan = Scan<size_t>(currentSpan[0]);
    	size_t secondSpan = Scan<size_t>(currentSpan[1]);
    	m_source_spans.push_back(std::make_pair(firstSpan,secondSpan));
    }

    for(int i = EndSpanList.begin(); i != EndSpanList.end(); i++)
    {
        vector<string> currentSpan= Tokenize(*i,"-");

        //each pair should contain exactly 2 elements
        CHECK(currentSpan.size() == 2);
        size_t firstSpan = Scan<size_t>(currentSpan[0]);
        size_t secondSpan = Scan<size_t>(currentSpan[1]);
        m_source_spans.push_back(std::make_pair(firstSpan,secondSpan));
     }

    //Check that pairs are sorted
    CHECK(IsSourceSorted());

    m_srcCept = columns[3];
    m_tgtCept = columns[4];
  }

  const string &GetSrcCept() { return m_srcCept; }
  const string &GetTgtCept() { return m_tgtCept; }
  size_t GetSentID()    { return m_sentID; }
  size_t GetSourceSpanList()  { return m_source_spans; }
  size_t GetTargetSpanList()  { return m_target_spans; }

  //FB : TODO : Should we move this somewhere else ?
  bool IsSourceSorted()
  {
	  for(i = m_source_spans.begin(); i != m_source_spans.end(); i++)
	  {
		  return (i.second < j.first);
	  }
  }

private:
  DWLLine();
  vector<pair<int,int> > m_source_spans;
  vector<pair<int,int> > m_target_spans;
  vector<string> m_source, m_target;
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
  TTableCollection ttables(argv[3]);

  ExtractorConfig config;
  config.Load(argv[4]);
  FeatureExtractor extractor(*ttables.GetTargetIndex(), config, true);
  VWFileTrainConsumer consumer(argv[5]);
  WritePhraseIndex(ttables.GetTargetIndex(), argv[6]);
  bool ttable_intersection = false;

  // parse options
  // TODO use some library to do this
  vector<size_t> toAnnotate;
  for (int i = 7; i < argc; i++) {
    string opt = argv[i];
    if (opt == "--annotate") {
      if (i + 1 >= argc) {
        cerr << "No argument given to --annotate" << endl;
        exit(1);
      }
      toAnnotate = Scan<size_t>(Tokenize(argv[++i], ","));
    } else if (opt == "--intersection") {
      ttable_intersection = true;
    } else {
      cerr << "Unknown option: " << opt << endl;
      exit(1);
    }
  }

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

    //FB : reimplement ttables into cepts
    if (! ttables.SrcExists(dwlLine.GetSrcCept()))
      continue;

    // we have all correct translations of the current phrase
    if (dwlLine.GetSrcCept() != srcCept || dwlLine.GetSrcStart() != spanStart || newSentence) {
      // generate features
      if (hasTranslation) {
        srcSurvived++;

        extractor.GenerateFeatures(&consumer, context, spanStart, spanEnd, translations, losses);
        newSentence = false;
      }

      // set new source phrase, context, translations and losses
      hasTranslation = false;
      srcCept = dwlLine.GetSrcCept();
      sourceSpanList = dwlLine.GetSourceSpanList();
      context = ReadFactoredLine(corpusLine, config.GetFactors().size());
      translations = ttables.GetAllTranslations(srcPhrase, ttable_intersection);
      losses.clear();
      losses.resize(translations.size(), 1);
      srcTotal++;
    }

    bool foundTgt = false;
    size_t tgtPhraseID = ttables.GetTgtPhraseID(dwlLine.GetTgtPhrase(), &foundTgt);

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
