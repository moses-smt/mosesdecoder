// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_GenerationDictionary_h
#define moses_GenerationDictionary_h

#include <list>
#include <stdexcept>
#include <vector>
#include <boost/unordered_map.hpp>
#include "ScoreComponentCollection.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "moses/FF/DecodeFeature.h"

namespace Moses
{

class FactorCollection;

typedef boost::unordered_map < Word , ScoreComponentCollection > OutputWordCollection;
// 1st = output phrase
// 2nd = log probability (score)

/** Implementation of a generation table in a trie.
 */
class GenerationDictionary : public DecodeFeature
{
  typedef boost::unordered_map<const Word* , OutputWordCollection, UnorderedComparer<Word>, UnorderedComparer<Word> > Collection;
protected:
  static std::vector<GenerationDictionary*> s_staticColl;

  Collection m_collection;
  // 1st = source
  // 2nd = target
  std::string						m_filePath;

public:
  static const std::vector<GenerationDictionary*>& GetColl() {
    return s_staticColl;
  }

  GenerationDictionary(const std::string &line);
  virtual ~GenerationDictionary();

  //! load data file
  void Load(AllOptions::ptr const& opts);

  /** number of unique input entries in the generation table.
  * NOT the number of lines in the generation table
  */
  size_t GetSize() const {
    return m_collection.size();
  }
  /** returns a bag of output words, OutputWordCollection, for a particular input word.
  *	Or NULL if the input word isn't found. The search function used is the WordComparer functor
  */
  const OutputWordCollection *FindWord(const Word &word) const;
  void SetParameter(const std::string& key, const std::string& value);

};


}
#endif
