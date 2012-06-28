/*******************************************
DAMT Hiero : Classifier
********************************************/

#ifndef moses_Classifier_h
#define moses_Classifier_h

#include <iostream>
#include <vector>
#include <iterator>
#include <string>
#include <set>
#include "ClassExample.h"
#include "util/check.hh"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "util/tokenize_piece.hh"

#include <boost/unordered_map.hpp>
#include "AlignmentInfoCollection.h"

namespace Moses
{
    //Singleton classifier
    class Classifier
    {
        typedef boost::unordered_map<ClassExample,
            float,
            ClassExampleKeyHasher,
            ClassExampleEqualityPred> PredictMap;

        private:
        static Classifier s_instance;
        Classifier(){};
        PredictMap m_predict;

        public :
        static Classifier Instance(){
            return s_instance;}

        void LoadScores(const std::string &ttableFile);

        float GetPrediction(const ClassExample& example);
    };
}
#endif
