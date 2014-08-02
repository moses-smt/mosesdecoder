#ifndef MERT_WIPOBLEU_SCORER_H_
#define MERT_WIPOBLEU_SCORER_H_

#include <ostream>
#include <string>
#include <vector>

#include "Types.h"
#include "ScoreData.h"
#include "StatisticsBasedScorer.h"
#include "ScopedVector.h"

#include "BleuScorer.h"

namespace MosesTuning
{

class WipoBleuScorer: public BleuScorer
{
  public:
    
    explicit WipoBleuScorer(const std::string& config = "");
    ~WipoBleuScorer();
    
    virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
    virtual bool OpenReferenceStream(std::istream* is, std::size_t file_id);
};

std::string preprocessWipo(const std::string&);

}

#endif 
