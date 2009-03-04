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
void ScoreIndexManager::AddScoreProducer(const ScoreProducer* sp)
{
	// Producers must be inserted in the order they are created
	const_cast<ScoreProducer*>(sp)->CreateScoreBookkeepingID();
	assert(m_begins.size() == (sp->GetScoreBookkeepingID()));

	m_producers.push_back(sp);
	if (sp->IsStateless()) {
		const StatelessFeatureFunction* ff = static_cast<const StatelessFeatureFunction*>(sp);
		if (!ff->ComputeValueInTranslationOption())
			m_stateless.push_back(ff);
	} else {
		m_stateful.push_back(static_cast<const StatefulFeatureFunction*>(sp));
	}

	m_begins.push_back(m_last);
	size_t numScoreCompsProduced = sp->GetNumScoreComponents();
	assert(numScoreCompsProduced > 0);
	m_last += numScoreCompsProduced;
	m_ends.push_back(m_last);
	/*VERBOSE(1,"Added ScoreProducer(" << sp->GetScoreBookkeepingID()
						<< " " << sp->GetScoreProducerDescription()
						<< ") index=" << m_begins.back() << "-" << m_ends.back()-1 << std::endl);
	*/
}

void ScoreIndexManager::Debug_PrintLabeledScores(std::ostream& os, const ScoreComponentCollection& scc) const
{
	std::vector<float> weights(scc.m_scores.size(), 1.0f);
	Debug_PrintLabeledWeightedScores(os, scc, weights);
}

void ScoreIndexManager::Debug_PrintLabeledWeightedScores(std::ostream& os, const ScoreComponentCollection& scc, const std::vector<float>& weights) const
{
  assert(m_featureNames.size() == weights.size());
  for (size_t i = 0; i < m_featureNames.size(); ++i)
    os << m_featureNames[i] << "\t" << weights[i] << endl;
}

void ScoreIndexManager::InitFeatureNames() {
	m_featureNames.clear();
	size_t cur_i = 0;
	size_t cur_scoreType = 0;
	while (cur_i < m_last) {
		size_t nis_idx = 0;
		bool add_idx = (m_producers[cur_scoreType]->GetNumInputScores() > 1);
		while (nis_idx < m_producers[cur_scoreType]->GetNumInputScores()){
			ostringstream os;
			os << m_producers[cur_scoreType]->GetScoreProducerDescription();
			if (add_idx)
				os << '_' << (nis_idx+1);
			m_featureNames.push_back(os.str());
			nis_idx++;
			cur_i++;
		}

		int ind = 1;
		add_idx = (m_ends[cur_scoreType] - cur_i > 1);
		while (cur_i < m_ends[cur_scoreType]) {
			ostringstream os;
			os << m_producers[cur_scoreType]->GetScoreProducerDescription();
			if (add_idx)
				os << '_' << ind;
			m_featureNames.push_back(os.str());
			++cur_i;
			++ind;
		}
		cur_scoreType++;
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
	os << "Stateless: " << sim.m_stateless.size() << "\tStateful: " << sim.m_stateful.size() << endl;
	return os;
}

}

