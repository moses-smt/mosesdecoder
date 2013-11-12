
#pragma once

#include <vector>
#include <string>
#include <map>

class Phrase;
class TargetPhrase;
class Scores;
class Sentence;

class FeatureFunction
{
public:
  static const std::vector<FeatureFunction*>& GetColl() {
    return s_staticColl;
  }
  static FeatureFunction &FindFeatureFunction(const std::string& name);
  static void Evaluate(const Phrase &source
                       , TargetPhrase &targetPhrase
                       , Scores &estimatedFutureScore);
  static void Initialize(const Sentence &source);
  static void CleanUp(const Sentence &source);

  static size_t GetTotalNumScores() {
    return s_nextInd;
  }

  FeatureFunction(const std::string line);
  virtual ~FeatureFunction();

  virtual void Load()
  {}

  virtual void InitializeForInput(const Sentence &source)
  {}

  virtual void CleanUpAfterSentenceProcessing(const Sentence &source)
  {}

  virtual void ReadParameters();
  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const = 0;

  size_t GetStartInd() const {
    return m_startInd;
  }
  size_t GetNumScores() const {
    return m_numScores;
  }
  const std::string &GetName() const {
    return m_name;
  }

protected:
  static std::vector<FeatureFunction*> s_staticColl;
  static size_t s_nextInd;
  static std::map<std::string, size_t> m_nameInd;

  std::vector<std::vector<std::string> > m_args;
  size_t m_numScores, m_startInd;
  std::string m_name;

  void ParseLine(const std::string &line, std::string &featureName);
  void CreateName(const std::string &featureName);
  void Register();

};

