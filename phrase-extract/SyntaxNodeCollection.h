/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

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

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "SyntaxNode.h"
#include "SyntaxTree.h"

namespace MosesTraining
{

/** A collection of SyntaxNodes organized by start and end position.
 *
 */
class SyntaxNodeCollection
{
public:
  SyntaxNodeCollection() : m_numWords(0) {}

  ~SyntaxNodeCollection();

  //! Construct and insert a new SyntaxNode.
  SyntaxNode *AddNode( int startPos, int endPos, const std::string &label );

  //! Return true iff there are one or more SyntaxNodes with the given span.
  bool HasNode( int startPos, int endPos ) const;

  //! Lookup the SyntaxNodes for a given span.
  const std::vector< SyntaxNode* >& GetNodes( int startPos, int endPos ) const;

  bool HasNodeStartingAtPosition( int startPos ) const;
  const std::vector< SyntaxNode* >& GetNodesByStartPosition( int startPos ) const;
  bool HasNodeEndingAtPosition( int endPos ) const;
  const std::vector< SyntaxNode* >& GetNodesByEndPosition( int endPos ) const;

  //! Get a vector of pointers to all SyntaxNodes (unordered).
  const std::vector< SyntaxNode* >& GetAllNodes() {
    return m_nodes;
  };

  //! Get the number of words (defined as 1 + the max end pos of any node).
  std::size_t GetNumWords() const {
    return m_numWords;
  }

  //! Clear the container (this deletes the SyntaxNodes).
  void Clear();

  //! Extract a SyntaxTree (assuming the collection's nodes constitute a tree).
  std::auto_ptr<SyntaxTree> ExtractTree();

private:
  typedef std::map< int, std::vector< SyntaxNode* > > InnerNodeIndex;
  typedef std::map< int, InnerNodeIndex > NodeIndex;

  // Not copyable.
  SyntaxNodeCollection(const SyntaxNodeCollection &);
  SyntaxNodeCollection &operator=(const SyntaxNodeCollection &);

  std::vector< SyntaxNode* > m_nodes;
  NodeIndex m_index;
  int m_numWords;
  std::vector< SyntaxNode* > m_emptyNode;

  InnerNodeIndex m_endPositionsIndex;
  InnerNodeIndex m_startPositionsIndex;
};

}  // namespace MosesTraining
