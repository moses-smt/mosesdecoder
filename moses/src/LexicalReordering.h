//#ifndef LEXICAL_REORDERING_H
//#define LEXICAL_REORDERING_H

#pragma once

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"
#include "FeatureFunction.h"

#include "LexicalReorderingTable.h"

namespace Moses
{

class Factor;
class Phrase;
class Hypothesis;
class InputType;

class LexicalReordering : public StatefulFeatureFunction {
public:
    typedef int ReorderingType;
    
    enum Direction {Forward, Backward, Bidirectional, Unidirectional = Backward};
    enum Condition {F,E,C,FE,FEC};
    
    LexicalReordering *CreateLexicalReorderingModel(std::vector<FactorType>& f_factors, 
                                                    std::vector<FactorType>& e_factors,
                                                    const std::string &modelType,
                                                    const std::string &filePath, 
                                                    const std::vector<float>& weights);
    virtual ~LexicalReordering();
    
    virtual size_t GetNumScoreComponents() const {
        return m_NumScoreComponents; 
    };
    
    virtual FFState* Evaluate(
                              const Hypothesis& cur_hypo,
                              const FFState* prev_state,
                              ScoreComponentCollection* accumulator) const;
    
    const FFState* EmptyHypothesisState() const;
    
    virtual std::string GetScoreProducerDescription() const;
    
    std::string GetScoreProducerWeightShortName() const {
        return "d";
    };
    
    virtual int             GetNumOrientationTypes() const = 0;
    virtual OrientationType GetOrientationType(Hypothesis*) const = 0;
    
    std::vector<float> CalcScore(Hypothesis* hypothesis) const;
    void InitializeForInput(const InputType& i){
        m_Table->InitializeForInput(i);
    }
    
    Score GetProb(const Phrase& f, const Phrase& e) const;
    //helpers
    static std::vector<Condition> DecodeCondition(Condition c);
    static std::vector<Direction> DecodeDirection(Direction d);
private:
    Phrase auxGetContext(const Hypothesis* hypothesis) const;
private:
    LexicalReorderingTable* m_Table;
    size_t m_NumScoreComponents;
    std::vector< Direction > m_Direction;
    std::vector< Condition > m_Condition;
    bool m_OneScorePerDirection;
    std::vector< FactorType > m_FactorsE, m_FactorsF, m_FactorsC;
    int m_MaxContextLength;
};

}
//#endif //LEXICAL_REORDERING_H
