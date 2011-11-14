#ifndef __SCORER_FACTORY_H
#define __SCORER_FACTORY_H

#include <vector>
#include <string>

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

#endif  // __SCORER_FACTORY_H
