#include "SparseReordering.h"

using namespace std;

namespace Moses 
{

SparseReordering::SparseReordering(const map<string,string>& config) 
{
  for (map<string,string>::const_iterator i = config.begin(); i != config.end(); ++i) {
    cerr << i->first << " " << i->second << endl;
  }
}


void SparseReordering::AddScores(
              const TranslationOption& topt,
               LexicalReorderingState::ReorderingType reoType,
               LexicalReorderingConfiguration::Direction direction,
               ScoreComponentCollection* scores) const 
{
}

} //namespace

