/*******************************************
DAMT Hiero : Classifier
********************************************/

#include "Classifier.h"

namespace Moses {

Classifier Classifier::s_instance;

void Classifier::LoadScores(const std::string &ttableFile)
{
  std::vector<FactorType> srcFactors;
  srcFactors.push_back(0);

  std::ifstream ttableIn(ttableFile.c_str());

  std::string lineOrig;
  size_t count = 0;

  while(getline(ttableIn, lineOrig)) {
    const std::string *line;

    std::vector<std::string> tokens;
    std::vector<std::string> lContext;

    std::vector<Word> sourceContext;
    std::vector<std::string> leftSourceContext;

    //FB : TODO : handle right context
    //std::vector<std::string> rightSourceContext;
    std::vector<std::string> sourceSide;
    std::vector<std::string> targetSide;
    //read in all scores but just keep p(e|f)
    std::vector<float> scoreVector;

    TokenizeMultiCharSeparator(tokens, *line , "|||" );

    if (tokens.size() != 4 && tokens.size() != 5) {
      std::cout << "Bad formatting of model file at : " << count << std::endl;
      abort();}

    const std::string &sourcePhraseString = tokens[0]
               , &targetPhraseString = tokens[1]
               , &scoreString        = tokens[2]
               , &alignString        = tokens[3];

    TokenizeMultiCharSeparator(lContext,sourcePhraseString,"###");
    Tokenize(sourceSide,sourcePhraseString);
    Tokenize(targetSide,targetPhraseString);
    Tokenize<float>(scoreVector,scoreString);

    std::string leftSource = lContext[0];
    Tokenize(leftSourceContext,leftSource);

    //Get source context into words : for factor representation
    std::vector<std::string>::iterator itr_sourceContext;
    for(itr_sourceContext = leftSourceContext.begin(); itr_sourceContext != leftSourceContext.end(); itr_sourceContext++)
    {
        Word contextWord;
        contextWord.CreateFromString(Input, srcFactors, *itr_sourceContext, 0);
        sourceContext.push_back(contextWord);
    }

    //TODO : alignments cannot be retrieved in string form : modify?
    std::set<std::pair<size_t,size_t> > alignmentInfo;
    for (util::TokenIter<util::AnyCharacter, true> token(alignString, util::AnyCharacter(" \t")); token; ++token) {
        util::TokenIter<util::AnyCharacter, false> dash(*token, util::AnyCharacter("-"));
        size_t sourcePos = boost::lexical_cast<size_t>(*dash++);
        size_t targetPos = boost::lexical_cast<size_t>(*dash++);
        alignmentInfo.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
    const AlignmentInfo * alignments = AlignmentInfoCollection::Instance().Add(alignmentInfo);
    const ClassExample key(sourceContext, sourceSide, targetSide, alignments);
    float pEgivenF = scoreVector.front();
    std::pair<PredictMap::iterator,bool> insResult;
    insResult = m_predict.insert(std::make_pair(key,pEgivenF));
   }
}

  float Classifier::GetPrediction(const ClassExample& ce)
  {
    float score = 0.0;
    PredictMap :: iterator itr_predict;
    if ((itr_predict = m_predict.find(ce)) != m_predict.end())
      {
          score = log(itr_predict->second);
          std::cout << "Prediction made : " << score << std::endl;
      }

    std::cout << "Returned score : " << score << std::endl;
    return score;
  }

}//end of namespace
