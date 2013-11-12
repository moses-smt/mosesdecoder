
#include <iostream>
#include "Global.h"
#include "InputFileStream.h"
#include "Util.h"
#include "check.h"

#include "FF/FeatureFunction.h"
#include "FF/DistortionScoreProducer.h"
#include "FF/WordPenaltyProducer.h"
#include "FF/PhrasePenalty.h"
#include "FF/TranslationModel/PhraseTableMemory.h"
#include "FF/TranslationModel/UnknownWordPenalty.h"
#include "FF/LM/InternalLM.h"
#include "FF/LM/SRILM.h"

using namespace std;

Global Global::s_instance;

Global::Global()
{}

Global::~Global()
{
  // TODO Auto-generated destructor stub
}

void Global::Init(int argc, char** argv)
{
  for (int i = 0; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "-f") {
      m_iniPath = argv[++i];
    } else if (arg == "-i") {
      m_inputPath = argv[++i];
    }
  }

  // input file
  if (m_inputPath.empty()) {
    m_inputStrme = &cin;
  } else {
    m_inputStrme = new Moses::InputFileStream(m_inputPath);
  }

  // read ini file
  Moses::InputFileStream iniStrme(m_iniPath);

  ParamList *paramList = NULL;
  string line;
  while (getline(iniStrme, line)) {
    line = Trim(line);
    if (line.find("[") == 0) {
      paramList = &m_params[line];
    } else if (line.find("#") == 0 || line.empty()) {
      // do nothing
    } else {
      paramList->push_back(line);
    }
  }

  timer.check("InitParams");
  InitParams();
  timer.check("InitFF");
  InitFF();
  timer.check("InitWeight");
  InitWeight();
  timer.check("Start Load");
  Load();
  timer.check("Finished Load");
  
}

bool Global::ParamExist(const std::string &key) const
{
  Params::const_iterator iter;
  iter = m_params.find(key);
  bool ret = (iter != m_params.end());
  return ret;
}

void Global::InitParams()
{
  if (ParamExist("[stack]")) {
    stackSize = Scan<size_t>(m_params["[stack]"][0]);
  } else {
    stackSize = 200;
  }

  if (ParamExist("[distortion-limit]")) {
    maxDistortion = Scan<int>(m_params["[distortion-limit]"][0]);
  } else {
    maxDistortion = 6;
  }
}

void Global::InitFF()
{
  ParamList &list =	m_params["[feature]"];

  for (size_t i = 0; i < list.size(); ++i) {
    string &line = list[i];
    cerr << "line=" << line << endl;

    FeatureFunction *ff = NULL;
    if (line.find("Distortion") == 0) {
      ff = new DistortionScoreProducer(line);
    } else if (line.find("WordPenalty") == 0) {
      ff = new WordPenaltyProducer(line);
    } else if (line.find("PhraseDictionaryMemory") == 0) {
      ff = new PhraseTableMemory(line);
    } else if  (line.find("UnknownWordPenalty") == 0) {
      ff = new UnknownWordPenalty(line);
    } else if  (line.find("PhrasePenalty") == 0) {
      ff = new PhrasePenalty(line);
    } else if  (line.find("InternalLM") == 0) {
      ff = new FastMoses::InternalLM(line);
    } else if  (line.find("SRILM") == 0) {
      ff = new FastMoses::SRILM(line);
    } else {
      cerr << "Unknown FF " << line << endl;
      abort();
    }
  }
}

void Global::InitWeight()
{
  weights.SetNumScores(FeatureFunction::GetTotalNumScores());
  ParamList &list =	m_params["[weight]"];

  for (size_t i = 0; i < list.size(); ++i) {
    string &line = list[i];
    cerr << "line=" << line << endl;

    vector<string> toks = TokenizeFirstOnly(line, "=");
    CHECK(toks.size() == 2);

    FeatureFunction &ff = FeatureFunction::FindFeatureFunction(toks[0]);

    vector<SCORE> featureWeights;
    Tokenize<SCORE>(featureWeights, toks[1]);
    CHECK(ff.GetNumScores() == featureWeights.size());
    weights.SetWeights(ff, featureWeights);
  }
}

void Global::Load()
{
  std::vector<PhraseTable*> pts;

  cerr << "Loading" << endl;
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetColl();
  for (size_t i = 0; i < ffs.size(); ++i) {
    FeatureFunction *ff = ffs[i];
    PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
    if (pt) {
      // load pt after other ff
      pts.push_back(pt);
    } else {
      cerr << ff->GetName() << endl;
      ff->Load();
    }
  }

  // load pt
  for (size_t i = 0; i < pts.size(); ++i) {
    cerr << pts[i]->GetName() << endl;
    pts[i]->Load();
  }
}
