// $Id$

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdio>
#include <cassert>
#include "Util.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"
#include "ScoreProducer.h"
#include "ScoreComponentCollection.h" // debugging

namespace Moses
{
using namespace std;

void ScoreIndexManager::AddScoreProducer(const ScoreProducer* sp)
{
	// Producers must be inserted in the order they are created
	const_cast<ScoreProducer*>(sp)->CreateScoreBookkeepingID();
	assert(m_begins.size() == (sp->GetScoreBookkeepingID()));

	m_producers.push_back(sp);

	m_begins.push_back(m_last);
	size_t numScoreCompsProduced = sp->GetNumScoreComponents();
	assert(numScoreCompsProduced > 0);
	m_last += numScoreCompsProduced;
	m_ends.push_back(m_last);
	InitFeatureNames();
	/*VERBOSE(1,"Added ScoreProducer(" << sp->GetScoreBookkeepingID()
						<< " " << sp->GetScoreProducerDescription()
						<< ") index=" << m_begins.back() << "-" << m_ends.back()-1 << std::endl);
	*/
}

void ScoreIndexManager::PrintLabeledScores(std::ostream& os, const ScoreComponentCollection& scores) const
{
	std::vector<float> weights(scores.m_scores.size(), 1.0f);
	PrintLabeledWeightedScores(os, scores, weights);
}

void ScoreIndexManager::PrintLabeledWeightedScores(std::ostream& os, const ScoreComponentCollection& scores, const std::vector<float>& weights) const
{
  assert(m_featureShortNames.size() == weights.size());
	string lastName = "";
  for (size_t i = 0; i < m_featureShortNames.size(); ++i)
	{
		if (i>0)
		{ 
			os << " ";
		}
		if (lastName != m_featureShortNames[i])
		{
			os << m_featureShortNames[i] << ": ";
			lastName = m_featureShortNames[i];
		}
    os << weights[i] * scores[i];
	}
}

void ScoreIndexManager::InitFeatureNames() {
	m_featureNames.clear();
	m_featureIndexes.clear();
	m_featureShortNames.clear();
	size_t cur_i = 0;
	size_t cur_scoreType = 0;
	while (cur_i < m_last) {
		size_t nis_idx = 0;
		bool add_idx = (m_producers[cur_scoreType]->GetNumInputScores() > 1);
		while (nis_idx < m_producers[cur_scoreType]->GetNumInputScores()){
			ostringstream os;
			//os << m_producers[cur_scoreType]->GetScoreProducerDescription();
			//if (add_idx)
				//os << '_' << (nis_idx+1);
			os << cur_i;
			const string &featureName = os.str();
			m_featureNames.push_back(featureName);
			m_featureIndexes[featureName] = m_featureNames.size() - 1;
			nis_idx++;
			cur_i++;
		}

		int ind = 1;
		add_idx = (m_ends[cur_scoreType] - cur_i > 1);
		while (cur_i < m_ends[cur_scoreType]) {
			ostringstream os;
			//os << m_producers[cur_scoreType]->GetScoreProducerDescription();
			//if (add_idx)
				//os << '_' << ind;
			os << cur_i;
			const string &featureName = os.str();
			m_featureNames.push_back(featureName);
			m_featureIndexes[featureName] = m_featureNames.size() - 1;
			m_featureShortNames.push_back( m_producers[cur_scoreType]->GetScoreProducerWeightShortName() );
			++cur_i;
			++ind;
		}
		cur_scoreType++;
	}
}

void ScoreIndexManager::InitFeatureNamesAles() {
	m_featureNames.clear();
	m_featureIndexes.clear();
	m_featureShortNames.clear();
	size_t globalIndex = 0;
	vector<const ScoreProducer *>::const_iterator it;

	for (it = m_producers.begin(); it != m_producers.end(); ++it) {
		size_t scoreCount = (*it)->GetNumInputScores();
		for (size_t i = 0; i < scoreCount; ++i) {
			ostringstream oStream;
			oStream << (*it)->GetScoreProducerDescription() << "_" << globalIndex;
			m_featureNames.push_back(oStream.str());
			m_featureIndexes[oStream.str()] = globalIndex;
			++globalIndex;
		}
	}
}

#ifdef HAVE_PROTOBUF
void ScoreIndexManager::SerializeFeatureNamesToPB(hgmert::Hypergraph* hg) const {
	for (size_t i = 0; i < m_featureNames.size(); ++i) {
		hg->add_feature_names(m_featureNames[i]);
	}
}
#endif

void ScoreIndexManager::InitWeightVectorFromFile(const std::string& fnam, vector<float>* m_allWeights) const {
	assert(m_allWeights->size() == m_featureNames.size());
	ifstream in(fnam.c_str());
	assert(in.good());
	char buf[2000];
	map<string, double> name2val;
	while (!in.eof()) {
		in.getline(buf, 2000);
		if (strlen(buf) == 0) continue;
		if (buf[0] == '#') continue;
		istringstream is(buf);
		string fname;
		double val;
		is >> fname >> val;
		map<string, double>::iterator i = name2val.find(fname);
		assert(i == name2val.end()); // duplicate weight name
		name2val[fname] = val;
	}
	assert(m_allWeights->size() == m_featureNames.size());
	for (size_t i = 0; i < m_featureNames.size(); ++i) {
		map<string, double>::iterator iter = name2val.find(m_featureNames[i]);
		if (iter == name2val.end()) {
			cerr << "No weight found found for feature: " << m_featureNames[i] << endl;
			abort();
		}
		(*m_allWeights)[i] = iter->second;
	}
}

std::ostream& operator<<(std::ostream& os, const ScoreIndexManager& sim)
{
	for (size_t i = 0; i < sim.m_featureNames.size(); ++i) {
		os << sim.m_featureNames[i] << endl;
	}
	os  << endl;
	return os;
}

const std::string &ScoreIndexManager::GetFeatureName(size_t fIndex) const
{
#ifndef NDEBUG
  if (m_featureNames.size() <= fIndex) {
      printf("%s: %i ScoreIndexManager::GetFeatureName(m_producers.size() = %zd, m_featureNames.size() = %zd, fIndex = %zd)\n", __FILE__, __LINE__, m_producers.size(), m_featureNames.size(), fIndex);
  }
#endif
	assert(m_featureNames.size() > fIndex);
	return m_featureNames[fIndex];
}
//! get the index of a feature by name
size_t ScoreIndexManager::GetFeatureIndex(const std::string& fName) const
{
	return m_featureIndexes.find(fName)->second;
}

}
