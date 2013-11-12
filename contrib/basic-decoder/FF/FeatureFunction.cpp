
#include <set>
#include "FeatureFunction.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "check.h"

using namespace std;

std::vector<FeatureFunction*> FeatureFunction::s_staticColl;
size_t FeatureFunction::s_nextInd = 0;
std::map<std::string, size_t> FeatureFunction::m_nameInd;

FeatureFunction::FeatureFunction(const std::string line)
  : m_numScores(1)
{
  s_staticColl.push_back(this);

  std::string featureName;

  ParseLine(line, featureName);
  CreateName(featureName);

  Register();
  cerr << m_name << "=" << m_startInd << "-" << (m_startInd+m_numScores-1) << endl;
}

FeatureFunction::~FeatureFunction()
{
  // TODO Auto-generated destructor stub
}

void FeatureFunction::ReadParameters()
{
  while (!m_args.empty()) {
    const vector<string> &args = m_args[0];
    SetParameter(args[0], args[1]);

    m_args.erase(m_args.begin());
  }

}

void FeatureFunction::SetParameter(const std::string& key, const std::string& value)
{

}

void FeatureFunction::ParseLine(const std::string &line, std::string &featureName)
{
  vector<string> toks;
  Tokenize(toks, line);

  featureName = toks[0];

  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = TokenizeFirstOnly(toks[i], "=");
    CHECK(args.size() == 2);

    if (args[0] == "num-features") {
      m_numScores = Scan<size_t>(args[1]);
    } else if (args[0] == "name") {
      m_name = args[1];
    } else {
      m_args.push_back(args);
    }
  }
}

void FeatureFunction::CreateName(const std::string &featureName)
{
  if (m_name.empty()) {
    std::map<std::string, size_t>::const_iterator iter;
    iter = m_nameInd.find(featureName);
    if (iter == m_nameInd.end()) {
      m_nameInd[featureName] = 0;
      m_name = featureName + SPrint(0);
    } else {
      size_t num = iter->second;
      m_name = featureName + SPrint(num);
    }
  }
}

void FeatureFunction::Register()
{
  m_startInd = s_nextInd;
  s_nextInd += m_numScores;
}



FeatureFunction &FeatureFunction::FindFeatureFunction(const std::string& name)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    FeatureFunction &ff = *s_staticColl[i];
    if (ff.GetName() == name) {
      return ff;
    }
  }

  throw "Unknown feature " + name;
}

void FeatureFunction::Evaluate(const Phrase &source
                               , TargetPhrase &targetPhrase
                               , Scores &estimatedFutureScore)
{
  Scores &scores = targetPhrase.GetScores();
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    FeatureFunction &ff = *s_staticColl[i];
    ff.Evaluate(source, targetPhrase, scores, estimatedFutureScore);
  }

}

void FeatureFunction::Initialize(const Sentence &source)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    FeatureFunction &ff = *s_staticColl[i];
    ff.InitializeForInput(source);
  }
}

void FeatureFunction::CleanUp(const Sentence &source)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    FeatureFunction &ff = *s_staticColl[i];
    ff.CleanUpAfterSentenceProcessing(source);
  }
}
