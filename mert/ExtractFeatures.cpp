#include "ExtractFeatures.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

namespace MosesTuning{

ExtractFeatures::ExtractFeatures(const std::string& config)
	: m_allowedRel (new std::map<std::string, bool>()),
    m_getExtraData(""),
    m_computeExtraFeatures(false),
    m_lemmaMap (new std::map<std::string, std::string>()),
	  m_MIModel (new std::map<std::string, std::vector<float> > ())
{
	string javaPath,modelFileARPA, modelFileMI, lemmaMapFile;
	InitConfig(config);

	javaPath=getConfig("javaPath");
	if(javaPath==""){
		cerr<<"provide javaPath to extract dependencies\n";
		exit(-1);
	}

	modelFileARPA=getConfig("modelFileARPA");
		if(modelFileARPA==""){
			cerr<<"provide modelFileARPA for extra feature\n";
			exit(-1);
		}

	modelFileMI=getConfig("modelFileMI");
	lemmaMapFile=getConfig("lemmaMapFile");
	if(lemmaMapFile!="")
		ReadLemmaMap(lemmaMapFile);
	if(modelFileMI!=""){
		ReadMIModel(modelFileMI);
	}


	m_getExtraData=getConfig("getExtraData");

	if(getConfig("computeExtraFeatures")=="true")
		m_computeExtraFeatures=true;

	//create Java VM and initalize the Relations.java wrapper for Stanford dependencies
		javaWrapper = Moses::CreateJavaVM::Instance(javaPath);
		//Create object
		JNIEnv *env =  javaWrapper->GetAttachedJniEnvPointer();

		env->ExceptionDescribe();

		jobject rel = env->NewObject(javaWrapper->GetRelationsJClass(), javaWrapper->GetDepParsingInitJId());
		env->ExceptionDescribe();
		jmethodID	initLP = 	env->GetMethodID(javaWrapper->GetRelationsJClass(), "InitLP","()V");
		env ->CallObjectMethod(rel,initLP);
		env->ExceptionDescribe();

		m_workingStanforDepObj = env->NewGlobalRef(rel);
		env->DeleteLocalRef(rel);
		javaWrapper->GetVM()->DetachCurrentThread();

		//LM object
		boost::shared_ptr<lm::ngram::Model> (new lm::ngram::Model(modelFileARPA.c_str())).swap(m_WBmodel);

}



void ExtractFeatures::ReadMIModel(string modelFileMI){
	using Moses::Tokenize;

	std::ifstream file(modelFileMI.c_str()); // (fileName);
	string line;
	string subcat="";
	std::vector<std::string> tokens;
	std::vector<float> scores;
	if(file.is_open()){
		while(getline(file,line)){
			// !!!! SPLIT doesn't work as it should -> problem with delimitors
			Tokenize(tokens, line);
			subcat = tokens[0]+" "+tokens[1]+" "+tokens[2];
			scores.push_back(std::atof(tokens[3].c_str()));
			scores.push_back(std::atof(tokens[4].c_str()));
			m_MIModel->insert(std::pair<std::string, vector<float> > (subcat,scores));
			tokens.clear();
			scores.clear();
		}
	}
}

void ExtractFeatures::ReadLemmaMap(string lemmaFile){
	using Moses::Tokenize;

	std::ifstream file(lemmaFile.c_str()); // (fileName);
	string line,word,lemma;
	std::vector<std::string> tokens;
	if(file.is_open()){
		while(getline(file,line)){
			// !!!! SPLIT doesn't work as it should -> problem with delimitors
			Tokenize(tokens, line);
			if(tokens.size()>1){
				m_lemmaMap->insert(std::pair<std::string, std::string > (tokens[0],tokens[1]));
			}
			tokens.clear();
		}
	}
}



std::string ExtractFeatures::FilterArg(std::string arg){
	boost::regex web("^bhttp|^bhttps|^bwww");
	boost::regex date("([0-9]+[.|-|/]?)+"); //some sort of date or other garbage
	boost::regex nr("[0-9]+");
	boost::regex prn("i|he|she|we|you|they|it|me|them");
	boost::regex par("\\(|\\)");
	//lowercase in place
	boost::algorithm::to_lower(arg);
	if(boost::regex_match(arg,web))
		return "WWW";
	if(boost::regex_match(arg,date))
			return "DDAATTEE";
	if(boost::regex_match(arg,nr))
			return "NNRR";
	if(boost::regex_match(arg,prn))
			return "PRN";
	if(boost::regex_match(arg,par))
			return "-LRB-";
	std::map<string,string>::iterator it;
	it = m_lemmaMap->find(arg);
	if(it!=m_lemmaMap->end()){
		return it->second;
	}
	else
		return arg; //no filter needed, use the original arg

}


std::string ExtractFeatures::CallStanfordDep(const std::string& parsedSentence) const{

	JNIEnv *env =  javaWrapper->GetAttachedJniEnvPointer();
	env->ExceptionDescribe();

	jmethodID methodId = javaWrapper->GetProcessSentenceJId();

	/**
	 * arguments to be passed to ProcessParsedSentenceJId:
	 * string: parsed sentence
	 * boolean: TRUE for selecting certain relations (list them with GetRelationListJId method)
	*/

	jstring jSentence = env->NewStringUTF(parsedSentence.c_str());
	jboolean jSpecified = JNI_TRUE;
	env->ExceptionDescribe();


	/**
	 * Call this method to get a string with the selected dependency relations
	 * Issues: method should be synchronize since sentences are decoded in parallel and there is only one object per feature
	 * Alternative should be to have one object per sentence and free it when decoding finished
	 */
	if (!env->ExceptionCheck()){
		//it's the same method id
		//VERBOSE(1, "CALLING JMETHOD ProcessParsedSentenceJId: " << env->GetMethodID(javaWrapper->GetRelationsJClass(), "ProcessParsedSentence","(Ljava/lang/String;Z)Ljava/lang/String;") << std::endl);
		jstring jStanfordDep = reinterpret_cast <jstring> (env ->CallObjectMethod(m_workingStanforDepObj,methodId,jSentence,jSpecified));

		env->ExceptionDescribe();

		//Returns a pointer to a UTF-8 string
		if(jStanfordDep != NULL){
		const char* stanfordDep = env->GetStringUTFChars(jStanfordDep, 0);
		std::string dependencies(stanfordDep);

		//how to make sure the memory gets released on the Java side?
		env->ReleaseStringUTFChars(jStanfordDep, stanfordDep);
		env->DeleteLocalRef(jSentence);

		env->ExceptionDescribe();
		javaWrapper->GetVM()->DetachCurrentThread();
		return dependencies;
		}
		std::cerr<< "jStanfordDep is NULL" << std::endl;

		//this would be deleted anyway once the thread detaches?
		if(jSentence!=NULL){
			env->DeleteLocalRef(jSentence);
			env->ExceptionDescribe();
		}

		javaWrapper->GetVM()->DetachCurrentThread(); //-> when jStanfordDep in null it already crashed?

		return "null";
	}
	javaWrapper->GetVM()->DetachCurrentThread();


	return "exception";
}

vector<vector<string> > ExtractFeatures::MakeTuples(const string& sentence, const string& depRel, const string& pos_string){
	using Moses::Tokenize;

	//cout<<ref<<endl;
	//cout<<dep<<endl;
	//const char *vinit[] = {"S", "SQ", "SBARQ","SINV","SBAR","VP"};
	const char *vinit[] = {"nsubj","nsubjpass","dobj","iobj"};
	for(int i=0;i<sizeof(vinit)/sizeof(vinit[0]);i++){
			(*m_allowedRel)[vinit[i]]=true;
		}

	vector<string> words;
	vector<string> dependencies;
	vector<string> pos; // part-of-speech tags for each word -> this could come from the dependencies string
	vector<vector<string> > dependencyTuples;
	Tokenize(words,sentence);
	Tokenize(dependencies,depRel);
	Tokenize(pos,pos_string);
	words.insert(words.begin(),"ROOT");
	pos.insert(pos.begin(),"ROOT");

	// model scores main arguments, prepositional arguments attached to verb or noun
	// should be extended to allow all types -> for now I will just extract the scores and concatenate them in the features.dat file
	int modelType=0;
	if(getConfig("argType") == "main")
			modelType=0;
	if(getConfig("argType") == "prepV")
		modelType=1;
	if(getConfig("argType") == "prepN")
			modelType=2;
	if(getConfig("argType") == "prepAll")
			modelType=3;


	int dep,gov;
	string rel;
	for(size_t i=0; i<dependencies.size();i=i+3){
		if(i+2 >= dependencies.size())
			continue;
		rel = dependencies[i+2];
		//relations from parser -> (dep,gov,rel)
		//LM model scores: (rel,gov,dep) where rel in (dobj,iobj,nsubj,nsubjpass)
		if((modelType !=0  and rel.substr(0,5) == "prep_") or
				// need to figure out if the head is a verb
				(modelType == 0 and m_allowedRel->find(rel)!=m_allowedRel->end())){
			//SHOULD LEMMATIZE
			dep = strtol (dependencies[i].c_str(),NULL,10);
			gov = strtol (dependencies[i+1].c_str(),NULL,10);

			vector<string> tuple;
			tuple.push_back(rel);
			tuple.push_back(FilterArg(words[gov]));
			tuple.push_back(FilterArg(words[dep]));

			if((modelType == 1 and pos.size() > gov and pos[gov].substr(0,1) == "V") or //prep argument of verb
					(modelType == 2 and pos.size() > gov and pos[gov].substr(0,1) == "N") or //prep argument of noun
					modelType == 3 or // prep argument of any gov
					modelType == 0) //main argument
				dependencyTuples.push_back(tuple);


		}
	}
	return dependencyTuples;
}

float ExtractFeatures::GetWBScore(vector<string>& depRel) const{
	using namespace lm::ngram;
	//Model model("//Users//mnadejde//Documents//workspace//Subcat//DepRelStats.en.100K.ARPA");
	//Model model(m_modelFileARPA.c_str());
	  //State stateSentence(m_WBmodel->BeginSentenceState()),state(m_WBmodel->NullContextState());
	  State out_state0;
	  const Vocabulary &vocab = m_WBmodel->GetVocabulary();
	  lm::WordIndex context[3];// = new lm::WordIndex[3];
	  //depRel = rel gov dep -> rel verb arg
	  context[0]=vocab.Index(depRel[1]);
	  context[1]=vocab.Index(depRel[0]);
	  context[2]=vocab.Index("<unk>");
	  lm::WordIndex arg = vocab.Index(depRel[2]);
	  float score;
	  score = m_WBmodel->FullScoreForgotState(context,context+2,arg,out_state0).prob;
	  //cout<<depRel[0]<<" "<<depRel[1]<<" "<<depRel[2]<<" "<<score<<endl;

	  //delete[] context;
	  return score;
}

float ExtractFeatures::GetMIScore(vector<string>& depRel) const{
	  //depRel = rel gov dep -> rel verb arg
		string tuple = depRel[0]+" "+depRel[1]+" "+depRel[2];
		std::map< std::string,std::vector<float> >::iterator it_MI;
		float score = 0.0;
		it_MI = m_MIModel->find(tuple);
			if(it_MI!=m_MIModel->end())
				score = it_MI->second[0];
	  return score;
}

vector<float> ExtractFeatures::ComputeScore(const string &sentence, const string &depRel, const string &pos){
	stringstream featureStr;
	vector<vector<string> > dependencyTuples;
	vector<vector<string> >::iterator tuplesIt;
	dependencyTuples = MakeTuples(sentence,depRel,pos);
	float scoreWBmodel=0.0, scoreMImodel=0.0;
	vector<float> feature_scores;

	for(tuplesIt=dependencyTuples.begin();tuplesIt!=dependencyTuples.end();tuplesIt++){
		scoreWBmodel+=GetWBScore(*tuplesIt);
		if (!m_MIModel->empty())
			scoreMImodel+=GetMIScore(*tuplesIt);
	}
	feature_scores.push_back(scoreWBmodel);
	if (!m_MIModel->empty())
		feature_scores.push_back(scoreMImodel);

	return feature_scores;
}

string ExtractFeatures::GetFeatureStr(const string &sentence, const string &depRel, const string &pos){
	stringstream featureStr;
	vector<float> feature_scores = ComputeScore(sentence, depRel, pos);
	for(size_t i=0; i<feature_scores.size(); i++)
		featureStr<<"HeadFeature= "<<feature_scores[i]<<" ";

	return featureStr.str();
}

string ExtractFeatures::GetFeatureNames(){
	stringstream featureNames;
	//0 means one score?? -> I hate this stupid code!!!
	featureNames<<"HeadFeature_0 Headfeature_1"; //this should be number of scores
	return featureNames.str();

}

//HANDLE CONFIG PARAMETERS FROM SCORER.CPP

void ExtractFeatures::InitConfig(const string& config)
{
//    cerr << "Scorer config string: " << config << endl;
  size_t start = 0;
  while (start < config.size()) {
    size_t end = config.find(",", start);
    if (end == string::npos) {
      end = config.size();
    }
    string nv = config.substr(start, end - start);
    size_t split = nv.find(":");
    if (split == string::npos) {
      //throw runtime_error("Missing colon when processing scorer config: " + config);
    	cerr<< "Missing colon when processing scorer config: " + config<<endl;
    	exit(-1);
    }
    const string name = nv.substr(0, split);
    const string value = nv.substr(split + 1, nv.size() - split - 1);
    cerr << "name: " << name << " value: " << value << endl;
    m_config[name] = value;
    start = end + 1;
  }
}

/**
   * Get value of config variable. If not provided, return default.
   */
std::string ExtractFeatures::getConfig(const std::string& key) const {
   std::map<std::string,std::string>::const_iterator i = m_config.find(key);
   if (i == m_config.end()) {
     return "";
   } else {
  	 return i->second;
   }
  }

}
