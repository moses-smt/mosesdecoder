/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh
 
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

#pragma once

#include <iostream>
#include <ostream>
#include <set>
#include <vector>
#include <cstdlib>

namespace Moses
{

class AlignmentInfoCollection;

// Collection of non-terminal/terminal alignment pairs, ordered by source index.
class AlignmentInfo
{
  friend std::ostream& operator<<(std::ostream &, const AlignmentInfo &);
  friend struct AlignmentInfoOrderer;
  friend class AlignmentInfoCollection;

 public:
  typedef std::set<std::pair<size_t,size_t> > CollType;
  typedef std::vector<size_t> NonTermIndexMap;
  typedef std::vector<size_t> TermIndexMap;
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const { return m_collection.begin(); }
  const_iterator end() const { return m_collection.end(); }

  // Provides a map from target-side to source-side non-terminal indices.
  // The target-side index should be the rule symbol index (counting terminals).
  // The index returned is the rule non-terminal index (ignoring terminals).
  const NonTermIndexMap &GetNonTermIndexMap() const {
    return m_nonTermIndexMap;
  }
  
  // only used for hierarchical models, contains terminal alignments
  const CollType &GetTerminalAlignments() const {
    return m_terminalCollection;
  }
  
  // for phrase-based models, this contains all alignments, for hierarchical models only the NT alignments
  const CollType &GetAlignments() const {
    return m_collection;
  }
  
  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;
  
 private:
  // AlignmentInfo objects should only be created by an AlignmentInfoCollection
  explicit AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs)
    : m_collection(pairs)
  {
    BuildNonTermIndexMap();
  }
  
  // use this for hierarchical models
  explicit AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs, int* indicator)
  { 
	// split alignment set in terminals and non-terminals
	std::set<std::pair<size_t,size_t> > terminalSet;
	std::set<std::pair<size_t,size_t> > nonTerminalSet;
	std::set<std::pair<size_t,size_t> >::iterator iter;
	for (iter = pairs.begin(); iter != pairs.end(); ++iter) {
		if (*indicator == 1) nonTerminalSet.insert(*iter);
		else terminalSet.insert(*iter);
		indicator++;
	}
	m_collection = nonTerminalSet;
	m_terminalCollection = terminalSet;
	
	BuildNonTermIndexMap();
  }
  
  void BuildNonTermIndexMap();

  CollType m_collection;
  CollType m_terminalCollection;
  NonTermIndexMap m_nonTermIndexMap;
};

// Define an arbitrary strict weak ordering between AlignmentInfo objects
// for use by AlignmentInfoCollection.
struct AlignmentInfoOrderer
{
  bool operator()(const AlignmentInfo &a, const AlignmentInfo &b) const {
	if (a.m_collection == b.m_collection)
	  return a.m_terminalCollection < b.m_terminalCollection;
    return a.m_collection < b.m_collection;
  }
};

}
