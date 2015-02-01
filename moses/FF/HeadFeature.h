#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include "util/exception.hh"
#include "CreateJavaVM.h"
#include "moses/Util.h"
#include <boost/thread/tss.hpp>
#include "moses/LM/Ken.h"
#include "lm/model.hh"

namespace Moses
{


class SyntaxNode;
class SyntaxTree;

typedef boost::shared_ptr<SyntaxTree> SyntaxTreePtr;
typedef boost::shared_ptr<SyntaxNode> SyntaxNodePtr;

//map the dep rel string to it's score or to some other structure holding the information
class DepRelMap : public  boost::unordered_map<std::string,float>{
public:
	virtual ~DepRelMap() {}
};

class StringHashMap : public  boost::unordered_map<std::string,DepRelMap::iterator>{
public:
	virtual ~StringHashMap() {}
};

class JniEnvPointer{
public:
	JniEnvPointer(CreateJavaVM *javaWrapper){
	this->env = javaWrapper->GetAttachedJniEnvPointer();
	this->javaWrapper = javaWrapper;
	}
	~JniEnvPointer(){
		javaWrapper->GetVM()->DetachCurrentThread();
	}
	JNIEnv* GetEnv(){
		return this->env;
	}
	void Detach(CreateJavaVM *javaWrapper){
		javaWrapper->GetVM()->DetachCurrentThread();
	}
private:
	JNIEnv *env;
	CreateJavaVM *javaWrapper;
};

class Counters {

public:
	int subtreeCacheHits;
	int depRelCacheHits;
	Counters():subtreeCacheHits(0),depRelCacheHits(0){}
	virtual ~Counters() {}
};
//typedef boost::unordered_map<std::string,std::string> StringHashMap;


//have to extend the state to keep track of dep rel pairs that have already been scored
//the pairs should be head-source_word_id dep_source_word_id
class SyntaxTreeState : public FFState
{
	SyntaxTreePtr m_tree;
	//hash scored pairs make it static -> one per class but updated with each hypothesis
	//could make the key a string -> will slow things down head-id-dep-id

	StringHashMap  &m_subtreeCache;
	DepRelMap &m_depRelCache;
	Counters &m_counters;

public:
  SyntaxTreeState(SyntaxTreePtr tree, StringHashMap &subtreeCache, DepRelMap &depRelCache, Counters &counters)
    :m_tree(tree)
		,m_subtreeCache(subtreeCache)
		,m_depRelCache(depRelCache)
		,m_counters(counters)
  {}

  SyntaxTreePtr GetTree() const {
      return m_tree;
  }

  StringHashMap& GetSubtreeCache() const {
        return m_subtreeCache;
    }

  DepRelMap& GetDepRelCache() const {
        return m_depRelCache;
    }

  Counters& GetCounters() const {
          return m_counters;
      }

  int Compare(const FFState& other) const;
};



class SyntaxNode
{
protected:
  int m_start, m_end;
  std::string m_label;
  std::vector< SyntaxNodePtr > m_children;
  boost::weak_ptr<SyntaxNode> m_parent;
  //maybe this should have been a pointer to a Word object
  std::string m_head;
  std::string m_headPOS;
  bool m_isTerminal; //if is terminal than read the child value and put in as m_head
  bool m_isOpen;
  bool m_hasHead;
  bool m_isInternal;
public:
  SyntaxNode( int startPos, int endPos, std::string label )
    :m_start(startPos) //maybe set start and end based on children and alignemnt -> might want to know the source words it covers
    ,m_end(endPos)
    ,m_label(label)
    ,m_parent(SyntaxNodePtr()) //,m_parent(0)
    ,m_head("")
		,m_headPOS("")
	,m_isTerminal(false)
	,m_isOpen(false)
	,m_isInternal(false)
	,m_hasHead(false){
  }

  ~SyntaxNode();

  int GetStart() const {
    return m_start;
  }
  int GetEnd() const {
    return m_end;
  }

  int GetSize(){
	  return m_children.size();
  }
  SyntaxNodePtr GetNChild(int n){
	  return m_children[n];
  }
  std::string GetLabel() const {
    return m_label;
  }

  std::string GetHead(){
    return m_head;
  }

  void SetHead(std::string head){
    m_head = head;
    m_hasHead = true;
  }

  std::string GetHeadPOS(){
    return m_headPOS;
  }

  void SetHeadPOS(std::string headPOS){
    m_headPOS = headPOS;
  }

  bool HasHead(){
  	return m_hasHead;
  }

  bool IsTerminal(){
	  return m_isTerminal;
  }

  void SetIsTerminal(bool isTerminal){
	  m_isTerminal = isTerminal;
  }

  bool IsOpen(){
  	  return m_isOpen;
    }

   void SetIsOpen(bool isOpen){
  	  m_isOpen = isOpen;
    }

   bool IsInternal(){
   	  return m_isInternal;
     }

   void SetIsInternal(bool isInternal){
   	  m_isInternal = isInternal;
     }

   boost::weak_ptr<SyntaxNode> GetParent() {
    return m_parent;
  }
  void SetParent(SyntaxNodePtr parent) {
    m_parent = parent;
  }
  void AddChild(SyntaxNodePtr child) {
    m_children.push_back(child);
  }
  const std::vector< SyntaxNodePtr > &GetChildren() const {
    return m_children;
  }

  int FindHeadChild(std::vector<std::string> headRule);
  //do I want to return the object or the index like above?
  SyntaxNodePtr FindFirstChild(std::string label) const;
  SyntaxNodePtr FindChildRecursively(std::string label) const;
};

class SyntaxTree
{

protected:
  std::vector<SyntaxNodePtr > m_nodes;
  //SyntaxNode* m_top; //root of tree fragment
  SyntaxNodePtr m_top;
  int m_size;
  SyntaxNodePtr m_attachTo; //where the next node should be attached -> index in m_nodes
  std::vector< SyntaxNodePtr > m_openNodes; //nodes that will be connected with previous hypothesis
  std::string m_rootedTree; //the entire subtree covered from the current LHS

  friend std::ostream& operator<<(std::ostream&, const SyntaxTree&);

public:

  SyntaxTree():m_seenDepRel(),m_rootedTree(""){
    //m_top = 0;  // m_top doesn't get set unless ConnectNodes is called.
    m_size = 0;
  }



  ~SyntaxTree();


  SyntaxNodePtr AddNode( int startPos, int endPos, std::string label );
  void AddNode(SyntaxNodePtr newNode);
  void AddOpenNode(SyntaxNodePtr newNode){
	  m_openNodes.push_back(newNode);
  }

  SyntaxNodePtr GetTop() {
    return m_top;
  }

  int GetSize(){
	  return m_size;
  }

  std::string GetRootedTree(){
  	return m_rootedTree;
  }

  void SetRootedTree(std::string subtree){
  	m_rootedTree = subtree;
  }

  std::vector<SyntaxNodePtr > GetOpenNodes(){
	  return m_openNodes;
  }


  /**get Tree property from hypothesis and build a syntax tree structure
   * input: syntax tree in Berkeley format e.g [NP [NNS !]]
   * input type: string
   * return: root to syntax tree structure
   * return type: SyntaxNode *
   * -> this structure should be build for each rule before decoding as Matthias suggested
   * -> or build for each hypothesis only once (no need to build for unused rules) -> mark hypothesis with a pointer to the structure if it exists
  */
  SyntaxNodePtr FromString(std::string internalTree, boost::shared_ptr< std::map<std::string, std::string> > m_lemmaMap);

  std::string ToString();
  std::string ToStringLevel(std::string &tree,int maxLevel);
  std::string ToStringNodeCount(int maxNodes);
  std::string ToStringHead();
  void ToString(SyntaxNodePtr newNode, std::stringstream &tree);
  void ToStringLevel(SyntaxNodePtr node, std::string &tree, int level, int maxLevel);
  void ToStringNodeCount(SyntaxNodePtr node, std::stringstream &tree, int *nodeCount, int maxNodes);
  void ToStringDynamic(SyntaxNodePtr newNode,std::vector< SyntaxTreePtr > *previousTrees, std::stringstream *tree);
  void ToStringHead(SyntaxNodePtr newNode, std::stringstream &tree);
  void SetHeadOpenNodes(std::vector< SyntaxTreePtr > previousTrees);
  void FindHeads(SyntaxNodePtr newNode,std::map<std::string, std::vector <std::string> > &headRules) const;

  //find the arguments
  std::string* FindObj() const;
  std::string* FindSubj() const;
  std::map<std::string, bool> m_seenDepRel; //all dep rel scored up to the current hypothesis -> should be used in Compare of FFState


  void Clear();
};


class HeadFeature : public StatefulFeatureFunction
{
public:

  HeadFeature(const std::string &line);

  void GetNewStanfordDepObj();

  void SetParameter(const std::string& key, const std::string& value);


  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  //I don't understand this
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
	  SyntaxTreePtr startTree(new SyntaxTree());
	  return new SyntaxTreeState(startTree,GetCache(),GetCacheDepRel(),GetCounters()); //&SyntaxTree());
  }


  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const {};
  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;



  void Load();

  void ReadHeadRules();
  void ReadProbArg();
  void ReadLemmaMap();

  std::string CallStanfordDep(std::string parsedSentence) const;
  void ProcessDepString(std::string depRelString, std::vector< SyntaxTreePtr > previousTrees,ScoreComponentCollection* accumulator) const;
  void CleanUpAfterSentenceProcessing(const InputType& source);
  float GetWBScore(std::vector< std::string >& depRel) const;

 StringHashMap &GetCache() const {

  	StringHashMap *cache;
  	  cache = m_cache.get();
  	  if (cache == NULL) {
  	    cache = new StringHashMap;
  	    m_cache.reset(cache);
  	  }
  	  assert(cache);
  	  return *cache;
  }

 StringHashMap &ResetCache() const {

   	StringHashMap *cache;
   	cache = new StringHashMap;
   	assert(cache);
   	m_cache.reset(cache);
   	return *cache;
   }

 DepRelMap &GetCacheDepRel() const {

	 DepRelMap *cache;
  	  cache = m_cacheDepRel.get();
  	  if (cache == NULL) {
  	    cache = new DepRelMap;
  	    m_cacheDepRel.reset(cache);
  	  }
  	  assert(cache);
  	  return *cache;
  }

 DepRelMap &ResetCacheDepRel() const {

	 DepRelMap *cache;
   	cache = new DepRelMap;
   	assert(cache);
   	m_cacheDepRel.reset(cache);
   	return *cache;
   }

 Counters &GetCounters() const {

 	 Counters *counters;
 	 counters = m_counters.get();
   	  if (counters == NULL) {
   	  	counters = new Counters;
   	    m_counters.reset(counters);
   	  }
   	  assert(counters);
   	  return *counters;
   }

 	Counters &ResetCounters() const {

	 Counters *counters;
	 	 counters = new Counters;
	 	 assert(counters);
    	m_counters.reset(counters);
    	return *counters;
    }

 	JniEnvPointer &GetJniEnvPointer() const{
 		JniEnvPointer *jniEnvPointer;
 		jniEnvPointer = m_JniEnvPointer.get();
 		if(jniEnvPointer==NULL){
 			jniEnvPointer = new JniEnvPointer(javaWrapper);
 			m_JniEnvPointer.reset(jniEnvPointer);
 		}
 		assert(jniEnvPointer);
 		return *jniEnvPointer;
 	}

protected:
  boost::shared_ptr< std::map<std::string, std::vector <std::string> > > m_headRules;
  boost::shared_ptr< std::map<std::string, float> > m_probArg;
  boost::shared_ptr<lm::ngram::Model> m_WBmodel;
  boost::shared_ptr< std::map<std::string, std::string> > m_lemmaMap;
  boost::shared_ptr< std::map<std::string, bool> > m_allowedNT;

	std::string m_headFile;
	std::string m_probArgFile;
	std::string m_modelFileARPA;
	std::string m_lemmaFile;
	std::string m_jarPath;
	//should do the initialization only once when the wrapper is loaded
	//could make it a singleton class
	mutable CreateJavaVM *javaWrapper;
	mutable unsigned long long m_counter;
	mutable unsigned long long m_counterDepRel;
	mutable unsigned long long m_cacheHits;
	mutable unsigned long long m_cacheDepRelHits;
	mutable boost::thread_specific_ptr<StringHashMap> m_cache;
	mutable boost::thread_specific_ptr<DepRelMap> m_cacheDepRel;
	mutable boost::thread_specific_ptr<Counters> m_counters;
	mutable boost::thread_specific_ptr<JniEnvPointer> m_JniEnvPointer;
	//have to take care with this -> one Feature instance per decoder which works multithreaded
	jobject workingStanforDepObj;

};


}

