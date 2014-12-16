/*
 * hierarchical.h
 *
 *  Created on: Jan 27, 2010
 *      Author: Nadi Tomeh - LIMSI/CNRS
 *      MT Marathon 2010, Dublin
 */

#ifndef HIERARCHICAL_H_
#define HIERARCHICAL_H_

#include <utility>
#include <map>
#include <set>
#include <vector>

namespace MosesTraining
{

// HPhraseVertex represents a point in the alignment matrix
typedef std::pair <int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef std::pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a std::vector of phrases
// the bool value indicates if the associated phrase is within the length limit or not
typedef std::vector < HPhrase > HPhraseVector;

// SentenceVertices represents all vertices that have the same positioning of all extracted phrases
// The key of the std::map is the English index and the value is a std::set of the foreign ones
typedef std::map <int, std::set<int> > HSenteceVertices;

} // namespace

#endif /* HIERARCHICAL_H_ */
