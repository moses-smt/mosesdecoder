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

#include "Pos.h"


using namespace std;
using namespace Moses;

namespace Josiah {
  
  static string ToString(const TagSequence& ws)
  {
    ostringstream os;
    for (TagSequence::const_iterator i = ws.begin(); i != ws.end(); ++i)
      os << (*i)->GetString() << ",";
    return os.str();
  }

  static ostream& operator<<(ostream& out, const TagSequence& ws)
  {
    out << ToString(ws);
    return out;
  }

template<class P>
static void getPosTags(const P& words,  TagSequence& tags, FactorType factorType) {
  for (typename P::const_iterator i = words.begin(); i != words.end(); ++i) {
    tags.push_back(i->operator[](factorType));
    
    /*cerr << "F0: " << *(i->operator[](0)) << endl;
    const Factor* f1 = i->operator[](1);
    if (f1) {
      cerr << "F1: " << *f1 << endl;
    } else {
      cerr << "F1: " << "missing" << endl;
  }*/
  }
}

static void getSegmentWords(const vector<Word>& words, const WordsRange& segment, vector<Word>& segmentWords) {
  for (size_t i = segment.GetStartPos(); i <= segment.GetEndPos(); ++i) {
    segmentWords.push_back(words[i]);
  }
}



float Josiah::PosFeatureFunction::computeScore(const Sample & sample) {
  m_sourceTags.clear();
  TagSequence targetTags;
  getPosTags(sample.GetSourceWords(), m_sourceTags, m_sourceFactorType);
  getPosTags(sample.GetTargetWords(), targetTags, m_targetFactorType);
  //cerr << "Source " << m_sourceTags << endl;
  //cerr << "Target " << targetTags << endl;
  return computeScore(m_sourceTags, targetTags);
}

float Josiah::PosFeatureFunction::getSingleUpdateScore( const Sample & s, const TranslationOption * option, const WordsRange & targetSegment )
{
  const WordsRange& sourceSegment = option->GetSourceWordsRange();
  TagSequence newTargetTags;
  getPosTags(option->GetTargetPhrase(), newTargetTags, m_targetFactorType);
  return getSingleUpdateScore(s, sourceSegment,targetSegment, newTargetTags);
}

float Josiah::PosFeatureFunction::getPairedUpdateScore( const Sample & s, const TranslationOption * leftOption, const TranslationOption * rightOption, const WordsRange & targetSegment, const Phrase & targetPhrase )
{
  //just treat this as one segment
  WordsRange sourceSegment(leftOption->GetStartPos(), rightOption->GetEndPos());
  TagSequence newTargetTags;
  getPosTags(targetPhrase, newTargetTags, m_targetFactorType);
  return getSingleUpdateScore(s, sourceSegment, targetSegment, newTargetTags);
}



float Josiah::PosFeatureFunction::getFlipUpdateScore( const Sample & s, const TranslationOption * leftTgtOption, const TranslationOption * rightTgtOption, const Hypothesis * leftTgtHyp, const Hypothesis * rightTgtHyp, const WordsRange & leftTargetSegment, const WordsRange & rightTargetSegment )
{
  pair<WordsRange,WordsRange> sourceSegments(leftTgtOption->GetSourceWordsRange(), rightTgtOption->GetSourceWordsRange());
  pair<WordsRange,WordsRange> targetSegments(leftTargetSegment, rightTargetSegment);
  
  return getFlipUpdateScore(s, sourceSegments, targetSegments);
}


TagSequence Josiah::PosFeatureFunction::getCurrentTargetTags(const Sample & s, const WordsRange & targetSegment ) const 
{
  vector<Word> segmentWords;
  getSegmentWords(s.GetTargetWords(), targetSegment, segmentWords);
  TagSequence tags;
  getPosTags(segmentWords,tags, m_targetFactorType);
  return tags;
}

TagSequence Josiah::PosFeatureFunction::getCurrentTargetTags(const Sample & s ) const
{
  TagSequence tags;
  getPosTags(s.GetTargetWords(), tags, m_targetFactorType);
  return tags;
}

bool Josiah::SourceVerbPredicate::operator ( )( const Factor * tag )
{
  const string& tagString = tag->GetString();
  //This works for TreeTagger de
  return !tagString.empty() && tagString[0] == 'V';
}

bool Josiah::TargetVerbPredicate::operator ( )( const Factor * tag )
{
  //This is for lopar en
  const string& tagString = tag->GetString();
  //cerr << tagString  << " " << (tagString.length() > 1 && !tagString.compare(0,2,"md")) << endl;
  return (!tagString.empty() && tagString[0] == 'v') || (tagString.length() > 1 && !tagString.compare(0,2,"md"));
}



float Josiah::VerbDifferenceFeature::computeScore( const TagSequence & sourceTags, const TagSequence & targetTags ) const
{
  SourceVerbPredicate svp;
  int sourceVerbs = (int)count_if(sourceTags.begin(), sourceTags.end(), svp);
  TargetVerbPredicate tvp;
  int targetVerbs = (int)count_if(targetTags.begin(), targetTags.end(), tvp);
  //cerr << "ComputeScore: source " << sourceVerbs << " target: " << targetVerbs << endl;
  return targetVerbs - sourceVerbs;
}

float Josiah::VerbDifferenceFeature::getSingleUpdateScore(const Moses::Sample& s, 
    const Moses::WordsRange& sourceSegment, const Moses::WordsRange& targetSegment, 
    const TagSequence& newTargetTags) const
{
  TargetVerbPredicate tvp;
  TagSequence oldTargetTags = getCurrentTargetTags(s, targetSegment);
  int oldTargetVerbs = (int)count_if(oldTargetTags.begin(), oldTargetTags.end(), tvp);
  int newTargetVerbs = (int)count_if(newTargetTags.begin(), newTargetTags.end(), tvp);
  //cerr << "SingleUpdate: new " << newTargetVerbs << " old " << oldTargetVerbs << endl;
  return newTargetVerbs - oldTargetVerbs;
}

}







