/*
 *  FeatureArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_ARRAY_H
#define FEATURE_ARRAY_H

#define FEATURES_TXT_BEGIN "FEATURES_TXT_BEGIN_0"
#define FEATURES_TXT_END "FEATURES_TXT_END_0"
#define FEATURES_BIN_BEGIN "FEATURES_BIN_BEGIN_0"
#define FEATURES_BIN_END "FEATURES_BIN_END_0"

using namespace std;

#include <limits>
#include <vector>
#include <iostream>
#include <fstream>

#include "Util.h"
#include "FeatureStats.h"

class FeatureArray
{
protected:
    featarray_t array_;
    size_t number_of_features;
    std::string features;

private:
    std::string idx; // idx to identify the utterance, it can differ from the index inside the vector

public:
    FeatureArray();

    ~FeatureArray() {};

    inline void clear() {
        array_.clear();
    }

    inline std::string getIndex() {
        return idx;
    }
    inline void setIndex(const std::string & value) {
        idx=value;
    }

    inline FeatureStats&  get(size_t i) {
        return array_.at(i);
    }
    inline const FeatureStats&  get(size_t i)const {
        return array_.at(i);
    }
    inline void set(FeatureStats& feat, size_t i) {
        array_.at(i)=feat;
    }
    void add(FeatureStats e) {
        array_.push_back(e);
    }

    void merge(FeatureArray& e);

    inline size_t size() {
        return array_.size();
    }
    inline size_t NumberOfFeatures() const {
        return number_of_features;
    }
    inline void NumberOfFeatures(size_t v) {
        number_of_features = v;
    }
    inline std::string Features() const {
        return features;
    }
    inline void Features(const std::string f) {
        features = f;
    }

    void savetxt(ofstream& outFile);
    void savebin(ofstream& outFile);
    void save(ofstream& outFile, bool bin=false);
    void save(const std::string &file, bool bin=false);
    inline void save(bool bin=false) {
        save("/dev/stdout",bin);
    }
    inline FeatureArray operator+(const FeatureArray feat)const {
        FeatureArray F;
        F.array_=array_;
	if (array_.size()!=feat.array_.size()) {
            cerr << "ERROR : FeatureArray::operator+ : different size "<<endl;
            exit(0);
        }for (int i =0; i< (int)array_.size(); i++) {
            F.array_.at(i)=F.array_.at(i)+feat.array_.at(i);
        } 
	F.features=features;
	F.idx=idx;
	F.number_of_features=number_of_features;
        return F;
    }

    void loadtxt(ifstream& inFile, size_t n);
    void loadbin(ifstream& inFile, size_t n);
    void load(ifstream& inFile);
    void load(const std::string &file);

    bool check_consistency();
    void applyLambda(float f);
};


#endif
