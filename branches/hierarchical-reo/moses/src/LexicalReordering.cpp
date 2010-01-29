#include <sstream>

#include "FFState.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "StaticData.h"

namespace Moses
{
    
LexicalReordering::LexicalReordering(std::vector<FactorType>& f_factors, 
                                     std::vector<FactorType>& e_factors,
                                     const std::string &modelType,
                                     const std::string &filePath, 
                                     const std::vector<float>& weights) {
    std::cerr << "Creating lexical reordering...\n";
    std::cerr << "weights: ";
    for(size_t w = 0; w < weights.size(); ++w){
        std::cerr << weights[w] << " ";
    }
    std::cerr << "\n";

    m_oneScorePerDirection = false; // default setting
    
    m_modelTypeString = modelType;
    m_modelType = Tokenize<std::string>(modelType, "-");
    std::vector<LexicalReordering::Condition> conditions;
    for(std::vector<std::string>::iterator it = m_modelType.begin(); it != m_modelType.end();) {
        if(DecodeDirection(*it) || DecodeCondition(*it) || DecodeNumFeatureFunctions(*it))
            it = m_modelType.erase(it);
        else
            ++it;
    }
 
    if(m_direction.empty())
        m_direction.push_back(Backward); // default setting
    
    for(size_t i = 0; i < m_condition.size(); ++i){
        switch(m_condition[i]){
            case E:
                m_factorsE = e_factors;
                if(m_factorsE.empty()){
                    //problem
                    UserMessage::Add("TL factor mask for lexical reordering is unexpectedly empty");
                    exit(1);
                }
                break;
            case F:
                m_factorsF = f_factors;
                if(m_factorsF.empty()){
                    UserMessage::Add("SL factor mask for lexical reordering is unexpectedly empty");
                    exit(1);
                }
                break;
            default:
                UserMessage::Add("Unknown conditioning option!");
                exit(1);
        }
    }
    
    size_t total_scores = 0;
    for(size_t i = 0; i < m_direction.size(); i++) {
        LexicalReorderingState *s = LexicalReorderingState::CreateLexicalReorderingState(m_modelType, m_direction[i]);
        m_scoreOffset.push_back(total_scores);
        total_scores += s->GetNumberOfScores();
        delete s;
    }
    
    if(m_oneScorePerDirection)
        m_numScoreComponents = m_direction.size();
    else
        m_numScoreComponents = total_scores;

    if(weights.size() != m_numScoreComponents) {
        std::ostringstream os;
        os << "Lexical reordering model (type " << modelType << "): expected " << m_numScoreComponents << " weights, got " << weights.size() << std::endl;
        UserMessage::Add(os.str());
        exit(1);
    }
    
    // add ScoreProducer - don't do this before our object is set up
    const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
    const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

    m_table = LexicalReorderingTable::LoadAvailable(filePath, m_factorsF, m_factorsE, std::vector<FactorType>());
}

LexicalReordering::~LexicalReordering() {
    if(m_table)
        delete m_table;
}

bool LexicalReordering::DecodeCondition(std::string configElement) {
    if(configElement == "f") {
        if(!m_condition.empty())
            goto double_spec;
        m_condition.push_back(F);
        return true;
    } else if(configElement == "fe") {
        if(!m_condition.empty())
            goto double_spec;
        m_condition.push_back(F);
        m_condition.push_back(E);
        return true;
    }
    
    return false;
    
double_spec:
    UserMessage::Add("Lexical reordering conditioning (f/fe) specified twice.");
    exit(1);
}

bool LexicalReordering::DecodeDirection(std::string configElement) {
    if(configElement == "backward" || configElement == "unidirectional") {
        if(!m_direction.empty())
            goto double_spec;
        m_direction.push_back(Backward);
        return true;
    } else if(configElement == "forward") {
        if(!m_direction.empty())
            goto double_spec;
        m_direction.push_back(Forward);
        return true;
    } else if(configElement == "bidirectional") {
        if(!m_direction.empty())
            goto double_spec;
        m_direction.push_back(Backward);
        m_direction.push_back(Forward);
        return true;
    }
    
    return false;
    
double_spec:
    UserMessage::Add("Lexical reordering direction (forward/backward/bidirectional) specified twice.");
    exit(1);
}    

bool LexicalReordering::DecodeNumFeatureFunctions(std::string configElement) {
    // not checking for double specification here for convenience
    if(configElement == "collapseff") {
        m_oneScorePerDirection = true;
        VERBOSE(1, "Collapsing reordering distributions into one feature function." << std::endl);
        return true;
    } else if(configElement == "allff") {
        m_oneScorePerDirection = false;
        VERBOSE(1, "Using one feature function for each orientation type." << std::endl);
        return true;
    }
    
    return false;
}

Scores LexicalReordering::GetProb(const Phrase& f, const Phrase& e) const {
    return m_table->GetScore(f, e, Phrase(Output));
}

FFState* LexicalReordering::Evaluate(const Hypothesis& hypo,
                                     const FFState* prev_state,
                                     ScoreComponentCollection* out) const {
    const FFStateArray *prev_states = dynamic_cast<const FFStateArray *> (prev_state);
    FFStateArray *next_states = new FFStateArray(prev_states->size());
    
    std::vector<float> score(GetNumScoreComponents(), 0);
    std::vector<float> values;
    
    //for every direction
    for(size_t i = 0; i < m_direction.size(); ++i) {
        ReorderingType reo;
        const LexicalReorderingState *prev = dynamic_cast<const LexicalReorderingState *> ((*prev_states)[i]);
        (*next_states)[i] = prev->Expand(hypo, reo);
        
        const Hypothesis *cache_hypo;
        switch (m_direction[i]) {
            case Forward:
                //TODO: still using GetPrevHypo here
                cache_hypo = hypo.GetPrevHypo();
                if(cache_hypo->GetId() == 0)
                    continue;
                
                break;
            case Backward:
                cache_hypo = &hypo;
                break;
            default:
                abort();
        }

        const ScoreComponentCollection &reorderingScoreColl = cache_hypo->GetCachedReorderingScore();
        values = reorderingScoreColl.GetScoresForProducer(this);
        assert(values.size() == (m_numScoreComponents));
     
        float value = values[reo + m_scoreOffset[i]];
        if(m_oneScorePerDirection) { 
            //one score per direction
            score[i] = value;
        } else {
            //one score per direction and orientation
            score[reo + m_scoreOffset[i]] = value; 
        }
    }
    
    out->PlusEquals(this, score);
    
    return next_states;
}

const FFState* LexicalReordering::EmptyHypothesisState() const {
    FFStateArray *states = new FFStateArray();
    for(size_t i = 0; i < m_direction.size(); i++)
        states->push_back(LexicalReorderingState::CreateLexicalReorderingState(m_modelType, m_direction[i]));
    return states;
}

}

