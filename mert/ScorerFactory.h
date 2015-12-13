#ifndef MERT_SCORER_FACTORY_H_
#define MERT_SCORER_FACTORY_H_

#include <vector>
#include <string>

namespace MosesTuning
{


class Scorer;

class ScorerFactory
{
public:
  static std::vector<std::string> getTypes();

  static Scorer* getScorer(const std::string& type, const std::string& config = "");

private:
  ScorerFactory() {}
  ~ScorerFactory() {}
};

}

#endif  // MERT_SCORER_FACTORY_H_
