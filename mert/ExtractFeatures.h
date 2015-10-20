#include "lm/model.hh"
#include <boost/shared_ptr.hpp>
#include "moses/FF/CreateJavaVM.h"
#include "moses/Util.h"


using namespace std;

/*
 * Compute features on nbest list to use for reranking
 * Call the functions in Data.cpp when extracting the score statistics for evaluation and features values for mert
 */

namespace MosesTuning{

class ExtractFeatures{
	public:
		 ExtractFeatures(const std::string& config="");
	  //~ExtractFeatures();

	  std::string CallStanfordDep(const std::string& parsedSentence) const;
	  vector<vector<string> > MakeTuples(const string& sentence, const string& dep, const string& pos);
	  float GetWBScore(vector<string>& depRel) const;
	  float GetMIScore(vector<string>& depRel) const;
	  vector<float> ComputeScore(const string &sentence, const string &depRel, const string& pos);
	  string GetFeatureStr(const string &sentence, const string &depRel, const string &pos);
	  string GetFeatureNames();
	  void InitConfig(const string& config);
	  std::string getConfig(const std::string& key) const;
	  void ReadMIModel(string modelFileMI);
	  void ReadLemmaMap(string lemmaMapFile);
	  std::string FilterArg(std::string word);

	  inline string getExtraData(){
	  	return m_getExtraData;
	  }
	  inline bool ComputeExtraFeatures(){
	  	    	return m_computeExtraFeatures;
	  	    }

	private:
	  mutable Moses::CreateJavaVM *javaWrapper;
	  jobject m_workingStanforDepObj;
	  boost::shared_ptr< std::map<std::string, std::string> > m_lemmaMap;
	  boost::shared_ptr< std::map<std::string, std::vector<float> > > m_MIModel;
	  //models
	  boost::shared_ptr<lm::ngram::Model> m_WBmodel;

	  boost::shared_ptr< std::map<std::string, bool> > m_allowedRel;

	  std::map<std::string, std::string> m_config;
	  string m_getExtraData;

	  bool m_computeExtraFeatures;
};

}
