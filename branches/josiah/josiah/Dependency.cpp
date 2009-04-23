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

#include "Dependency.h"

using namespace Moses;
using namespace std;

namespace Josiah {

static void addChildren(vector<set<size_t> >& tree, size_t parent, set<size_t>& children) {
  for (set<size_t>::const_iterator i = tree[parent].begin(); i!= tree[parent].end(); ++i) {
    children.insert(*i);
    addChildren(tree,*i,children);
  }
}
  
DependencyTree::DependencyTree(const vector<Word>& words, FactorType parentFactor) {
  vector<set<size_t> > tree(words.size()); // map parents to their immediate children
  size_t root = -1;
  for (size_t child = 0; child < words.size(); ++child) {
    int parent = atoi(words[child][parentFactor]->GetString().c_str());
    if (parent < 0) {
      root = child;
    } else {
      tree[(size_t)parent].insert(child);
    }
    m_parents.push_back(parent);
  }
  m_spans.resize(words.size());
  for (size_t i = 0; i < m_parents.size(); ++i) {
    addChildren(tree,i,m_spans[i]);
  }
  
}

static string ToString(const DependencyTree& t)
{
  ostringstream os;
  for (size_t i = 0; i < t.getLength(); ++i) {
    os << i << "->" << t.getParent(i) << ", ";
  }
  return os.str();
}

ostream& operator<<(ostream& out, const DependencyTree& t)
{
  out << ToString(t);
  return out;
}

/** Parent of this index, -1 if root*/
int DependencyTree::getParent(size_t index) const {
  return m_parents[index];
}

/** Does the parent word cover the child word? */
bool DependencyTree::covers(size_t parent, size_t descendent) const {
  return m_spans[parent].count(descendent);
}

float CherrySyntacticCohesionFeature::computeScore() {return 0;}
/** Score due to  one segment */
float CherrySyntacticCohesionFeature::getSingleUpdateScore(const TranslationOption* option, const WordsRange& targetSegment) {return 0;}
/** Score due to two segments **/
float CherrySyntacticCohesionFeature::getPairedUpdateScore(const TranslationOption* leftOption,
    const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) {return 0;}

/** Score due to flip */
float CherrySyntacticCohesionFeature::getFlipUpdateScore(const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                                     const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                                     const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) {return 0;}

}
