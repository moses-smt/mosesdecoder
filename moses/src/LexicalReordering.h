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
    enum Direction {Forward, Backward};
    enum Condition {F,E,FE};
    
    LexicalReordering(std::vector<FactorType>& f_factors, 
                      std::vector<FactorType>& e_factors,
                      const std::string &modelType,
                      const std::string &filePath, 
                      const std::vector<float>& weights);
    virtual ~LexicalReordering();
    
    virtual size_t GetNumScoreComponents() const {
        return m_numScoreComponents; 
    };
    
    virtual FFState* Evaluate(const Hypothesis& cur_hypo,
                              const FFState* prev_state,
                              ScoreComponentCollection* accumulator) const;
    
    const FFState* EmptyHypothesisState() const;
    
    virtual std::string GetScoreProducerDescription() const;
    
    std::string GetScoreProducerWeightShortName() const {
        return "d";
    };
    
    std::vector<float> CalcScore(Hypothesis* hypothesis) const;
    void InitializeForInput(const InputType& i){
        m_table->InitializeForInput(i);
    }
    
    Score GetProb(const Phrase& f, const Phrase& e) const;
    
private:
    bool DecodeCondition(std::string s);
    bool DecodeDirection(std::string s);
    bool DecodeNumFeatureFunctions(std::string s);

    std::string m_modelTypeString;
    std::vector<std::string> m_modelType;
    LexicalReorderingTable* m_table;
    size_t m_numScoreComponents;
    std::vector<Direction> m_direction;
    std::vector<Condition> m_condition;
    std::vector<size_t> m_scoreOffset;
    bool m_oneScorePerDirection;
    std::vector<FactorType> m_factorsE, m_factorsF;
};

}
//#endif //LEXICAL_REORDERING_H
