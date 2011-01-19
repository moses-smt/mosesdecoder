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

using namespace std;

// HPhraseVertex represents a point in the alignment matrix
typedef pair <int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a vector of phrases
// the bool value indicates if the associated phrase is within the length limit or not
typedef vector < HPhrase > HPhraseVector;

// SentenceVertices represents all vertices that have the same positioning of all extracted phrases
// The key of the map is the English index and the value is a set of the foreign ones
typedef map <int, set<int> > HSenteceVertices;


#endif /* HIERARCHICAL_H_ */
