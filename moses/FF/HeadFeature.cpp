#include <vector>
#include "HeadFeature.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"



#include "moses/StaticData.h"
#include "moses/ChartHypothesis.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/TreeStructurePhraseProperty.h"


#include <iostream>
#include <fstream>
#include <math.h>
#include <locale>
#include <boost/regex.hpp>

//#include </System/Library/Frameworks/JavaVM.framework/Headers/jni.h>



using namespace std;


namespace Moses
{

// MARIA //

void split(std::string &s, std::string delim, std::vector<std::string> &tokens) {
	const std::locale& Loc =std::locale();
	boost::trim_if(s,::boost::is_space(Loc));
	boost::find_format_all(s,
			boost::token_finder(::boost::is_space(Loc), boost::token_compress_on),
			 boost::const_formatter(boost::as_literal(" ")));
    boost::split(tokens, s, boost::is_any_of(delim));
}

SyntaxNode::~SyntaxNode(){
	m_label.clear();
	m_head.clear();
	//delete children ? how without delteing it twice
	//delete parent
}

SyntaxNodePtr SyntaxNode::FindFirstChild(std::string label) const{
	for(int i=0; i<m_children.size();i++){
		if(m_children[i]->m_label.compare(label)==0)
				return m_children[i];
	}
	return SyntaxNodePtr();
}


SyntaxNodePtr SyntaxNode::FindChildRecursively(std::string label) const{
	SyntaxNodePtr child = this->FindFirstChild(label);
	//we want to avoid the case [Q [Q] [VP]] -> the VP gets extended with the previous hypothesis and the pair is scored then
	if(child && child->IsInternal())
		return child;
	else{
		for(int i=0; i<m_children.size();i++){
			if(m_children[i]->m_isInternal){ //so it doesn't recurse past the leaves of the current hypothesis tree
				child = m_children[i]->FindChildRecursively(label);
				if(child)
					return child; //!!! check this  recursivity
			}
		}
	}
	return SyntaxNodePtr();
}

int SyntaxNode::FindHeadChild(std::vector<std::string> headRule){
	int i,j;
	//should not look at terminals -> eg [NP Ich]
	// There is a problem [VP [VBP] [NP]] takes head from NP -> why?? check comparison and Head_rules
	//even with VP 1 VB VBP... NP I still get the head from the NP -> why?
	if(headRule[0].compare("1")==0){
		if(headRule[1].compare("**")==0){
			if(m_children.size()>0)
				return 0; //return first child
			else
				return -1; //no children for this node -> should be treated elsewhere
		}
		//!!! PROBLEM !!!
		// PP    1       NP NN NNP NNPS NNS IN TO VBG VBN RP FW
		// the first child will be TO or IN so it will always find that first in the list
		//instead I should loop outside on the rule and inside on the children
		// that way I find first NP
		// or change directation to 0 and look for the last children first -> this way the head is the NP not the function word
		// originally Collins rule: PP	1	IN TO VBG VBN RP FW	 -> so maybe I should leave this as head and just look again for first NP, NN recursively

		// From standford parser class for finding heads:
		//"left" means search left-to-right by category and then by position
		//"leftdis" means search left-to-right by position and then by category
		//"right" means search right-to-left by category and then by position
		//"rightdis" means search right-to-left by position and then by category
		//"leftexcept" means to take the first thing from the left that isn't in the list
		//"rightexcept" means to take the first thing from the right that isn't on the list

		for(i=0; i<m_children.size();i++){
			for(j=1; j<headRule.size();j++){ //first item is direction
				if(m_children[i]->m_label.compare(headRule[j])==0){
					return i;
				}
			}
		}
	}
	//have to figure out what this 0 means -> VP 0 .. looks wrong
	//!!! left and right is for the direction of the children not the rule table ->
	//!!! corect the HEAD_RULES files
	if(headRule[0].compare("0")==0){
		if(headRule[1].compare("**")==0)
		 return m_children.size()-1; //return last child
		for(i=m_children.size()-1; i>=1;i--){
			for(j=1; j<headRule.size();j++)
				if(m_children[i]->m_label.compare(headRule[j])==0){
					return i;
				}
		}
	}
	return -1;
}

SyntaxTree::~SyntaxTree()
{
//with shared_ptr I shouldn't need this
  //Clear();
}

void SyntaxTree::Clear()
{
  //m_top = 0;
  // loop through all m_nodes, delete them
  for(size_t i=0; i<m_nodes.size(); i++) {
    m_nodes[i].reset();//delete m_nodes[i];
  }
  m_nodes.clear();

}


// I have no idea what this is supposed to do -> track source words covered? or target words
// would be useful to know the index in the target sentence for when scoring dependency pairs (avoid duplicates)
SyntaxNodePtr SyntaxTree::AddNode( int startPos, int endPos, std::string label )
{
	SyntaxNodePtr newNode (new SyntaxNode( startPos, endPos, label ));
  m_nodes.push_back( newNode );
  m_size ++;// std::max(endPos+1, m_size);
  return newNode;
}

void SyntaxTree::AddNode( SyntaxNodePtr newNode)
{
	if(m_size==0)
		m_top = newNode;
	else{
		newNode->SetParent(m_attachTo);
		m_attachTo->AddChild(newNode);
	}
	m_nodes.push_back( newNode );
	m_size ++;
}

std::string FilterArg(std::string arg, boost::shared_ptr< std::map<std::string, std::string> > m_lemmaMap){
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
		return ""; //no filter needed, use the original arg

}

SyntaxNodePtr SyntaxTree::FromString(std::string internalTree, boost::shared_ptr< std::map<std::string, std::string> > lemmaMap)
{
	size_t pos = 0;
	size_t nextpos = 0;
	char token = 0;
	std::vector<std::string> nodes;
	std::string temp;

	while(pos != internalTree.size()){
		token = internalTree[pos];
		pos++;
		if(token == '['){
			SyntaxNodePtr newNode;
			nextpos = internalTree.find_first_of("][",pos);
			nodes.clear();
			std::string temp = internalTree.substr(pos,nextpos-pos);
			split(temp,"\t ",nodes);
			if(nodes.size()==0)
				std::cerr<<" syntax string is not well formed: empty node"<<std::endl;
			if(internalTree[nextpos]==']'){
				SyntaxNodePtr(new SyntaxNode( 0, 0, nodes[0])).swap(newNode);
				//can have [TOP [S] ] pr [NP [PRP I]]
				if(nodes.size()==2){
					//try to lemmatize and filter
					std::string head = FilterArg(nodes[1],lemmaMap);
					std::string headPOS = nodes[0]; //we are in the preterminat case here so the label of the node will be the same as the POS of the head
					if(head!=""){
						newNode->SetHead(head);
						newNode->SetHeadPOS(headPOS);
					}
					else{
						newNode->SetHead(nodes[1]);
						newNode->SetHeadPOS(headPOS);
					}
					newNode->SetIsTerminal(true);
				}
				else{
					AddOpenNode(newNode); //nodes that will be connected with prev hypos
					newNode->SetIsOpen(true);
				}
				AddNode(newNode);
			}
			// there is a case due to faulty internal structure -> [Q <s> [S] </s>] so nodes.size() might be 2
			if(internalTree[nextpos]=='['){ // && nodes.size()==1){
				SyntaxNodePtr (new SyntaxNode( 0, 0, nodes[0])).swap(newNode);
				//internal tree -> expecting a child
				newNode->SetIsInternal(true);
				AddNode(newNode);
			}
			m_attachTo = newNode;
			pos = nextpos;
		}
		if(token == ']'){
			SyntaxNodePtr parent = m_attachTo->GetParent().lock();
			if(parent)
				m_attachTo = parent;
		}
	}

	return m_top;
}

void SyntaxTree::ToString(SyntaxNodePtr node, std::stringstream &tree){
	tree <<"("<< node->GetLabel()<< " ";
	//keep counter and for each index save the pointer to the node
	//-> this way I can keep track of seen dependency pairs (i get head-index dep-index pairs from StanfordDep, where index is relative to the subtree)
	//index seen pairs by head pointer and then have a list of dependent pointers ? or a hash somewhow?
	if(node->IsTerminal())
		tree << node->GetHead() << ")";
	else{
		for(int i=0;i<node->GetSize();i++)
			ToString(node->GetNChild(i),tree);
		tree << ")";
	}

}

void SyntaxTree::ToStringLevel(SyntaxNodePtr node, std::stringstream &tree, int level, int maxLevel){
	tree <<"("<< node->GetLabel()<< " ";
	//keep counter and for each index save the pointer to the node
	//-> this way I can keep track of seen dependency pairs (i get head-index dep-index pairs from StanfordDep, where index is relative to the subtree)
	//index seen pairs by head pointer and then have a list of dependent pointers ? or a hash somewhow?
	if(node->IsTerminal())
		tree << node->GetHead() << ")";
	else{
		if(level==maxLevel){
			//tree << "("<<node->GetHeadPOS()<<" "<< node->GetHead() <<")" << ")";
			tree << node->GetHead() << ")";
		}
		else{
			level++;
			for(int i=0;i<node->GetSize();i++)
				ToStringLevel(node->GetNChild(i),tree,level,maxLevel);
			tree << ")";
		}
	}

}

void SyntaxTree::ToStringNodeCount(SyntaxNodePtr node, std::stringstream &tree, int *nodeCount, int maxNodes){
	tree <<"("<< node->GetLabel()<< " ";
	//keep counter and for each index save the pointer to the node
	//-> this way I can keep track of seen dependency pairs (i get head-index dep-index pairs from StanfordDep, where index is relative to the subtree)
	//index seen pairs by head pointer and then have a list of dependent pointers ? or a hash somewhow?
	if(node->IsTerminal() ) //will add all the children even if the nodeCount is over maxNodes -> maxnNodes+nr of remaining children
		tree << node->GetHead() << ")";
	else{
		if(*nodeCount >= maxNodes){
			tree << "("<<node->GetHeadPOS()<<" "<< node->GetHead() <<")" << ")";
		}
		else{
			for(int i=0;i<node->GetSize();i++){
				(*nodeCount)++;
				//this way the tree will end up having more children (grandchildren) on the left side -> it's DFS
				//tree will grow as much in depth for the first child and the rest will stop imediatly because nodeCount will have been consumed
				//have to change to BFS
				ToStringNodeCount(node->GetNChild(i),tree,nodeCount,maxNodes);
			}
			tree << ")";
		}
	}

}

void SyntaxTree::ToStringDynamic(SyntaxNodePtr node, std::vector< SyntaxTreePtr > *previousTrees, std::stringstream *tree){
	//tree <<"("<< node->GetLabel()<< " ";
	//keep counter and for each index save the pointer to the node
	//-> this way I can keep track of seen dependency pairs (i get head-index dep-index pairs from StanfordDep, where index is relative to the subtree)
	//index seen pairs by head pointer and then have a list of dependent pointers ? or a hash somewhow?
	if(node->IsTerminal()){
		*tree <<"("<< node->GetLabel()<< " " << node->GetHead() << ")";
		if(node->IsOpen()) //the open node was extended with a leaf and set as IsTerminal. Still we have to pop the prev tree (we don't have anything to process since it was a leaf)
			previousTrees->erase(previousTrees->begin());
	}
	else{
		if(node->IsOpen() && !previousTrees->empty()){
					*tree << previousTrees->front()->GetRootedTree();
					previousTrees->erase(previousTrees->begin()); //
					}
		else{ //if(node->IsInternal()){
			*tree <<"("<< node->GetLabel()<< " ";
			//this is totally wrong -> it inverses the order of the children
			for(int i=0 ; i<node->GetSize();i++){
				ToStringDynamic(node->GetNChild(i),previousTrees,tree);
			}
			*tree << ")";
		}
	}
}

void SyntaxTree::ToStringHead(SyntaxNodePtr node, std::stringstream &tree){
	tree <<"("<< node->GetLabel()<<"-"<<node->GetHead()<< " ";
	if(node->IsTerminal())
		tree << node->GetHead() << ")";
	else{
		for(int i=0;i<node->GetSize();i++)
			ToStringHead(node->GetNChild(i),tree);
		tree << ")";
	}

}

std::string SyntaxTree::ToString(){
	std::stringstream tree("(");
	ToString(m_top,tree);
	return tree.str();
}

//using this might also solve the problem of scoring dep rel twice
//ex: VP1 -VP NP -> rel_obj  ; S -> NP VP1 -> rel_subj rel_obj
//if instead of the VP1 subtree we put only the head there should be no rel_obj extracted at the S node
std::string SyntaxTree::ToStringLevel(int maxLevel){
	std::stringstream tree("(");
	ToStringLevel(m_top,tree,1,maxLevel);
	return tree.str();
}

std::string SyntaxTree::ToStringNodeCount(int maxNodes){
	std::stringstream tree("(");
	int nodeCount =1;
	ToStringNodeCount(m_top,tree,&nodeCount,maxNodes);
	return tree.str();
}



std::string SyntaxTree::ToStringHead(){
	std::stringstream tree("(");
	ToStringHead(m_top,tree);
	return tree.str();
}



//!!! still problems with finding the head
// I had NP in the VP rule and it would always get that one first -> the 0/1 reversing might not be working or some if/else
void SyntaxTree::FindHeads(SyntaxNodePtr node, std::map<std::string, std::vector <std::string> > &headRules) const {
	std::map<std::string,std::vector<std::string> >::iterator it;
	//so I don't recurse through the children subtrees that have already been updated with the head
	if(node->HasHead())
		return;
	it = headRules.find(node->GetLabel());
	int child = -1;

	//the above check stops recursing through children that belong to previous hypothesis -> have already been updated
	for(int i=0; i<node->GetSize(); i++){
		FindHeads(node->GetNChild(i),headRules);
	}
	if( it!= headRules.end()){
		//std::cout<<"rule: "<<it->first<<std::endl;
		///!!!!//
		//[VP [VBP] [NP]]  -> why is the head taken from NP and not from VP ??
		child=node->FindHeadChild(it->second);
		//return 0 or size-1 for ** -> check if it has children
		//if it doesn't have children got to prev hyp

		///!!! I get VB a lot -> should it be VBD??
		if(child!=-1){
			//std::cout<<"Head: "<<node->GetNChild(child)->GetHead()<<std::endl;
			node->SetHead(node->GetNChild(child)->GetHead());
			node->SetHeadPOS(node->GetNChild(child)->GetHeadPOS());
		}
		else{
			if(node->GetSize()>0){
				//std::cout<<"Head: "<<node->GetNChild(0)->GetHead()<<std::endl;
				node->SetHead(node->GetNChild(0)->GetHead());
				node->SetHeadPOS(node->GetNChild(0)->GetHeadPOS());
			}
			//else
			//	std::cout<<"Head: no children: "<<node->GetHead()<<std::endl;
		}
		//!!!! if I find the rule but not any child I should still select the first child!
	}
	else{
		if(node->GetLabel().compare("NP")==0){
			it = headRules.find("NP1");
			//breaks because it doesn't check for it!=end
			//but the problem is headRules gets deleted???
			if( it!= headRules.end()) child=node->FindHeadChild(it->second);
			if(child==-1){
				it = headRules.find("NP2");
				if( it!= headRules.end()) child=node->FindHeadChild(it->second);
				if(child==-1){
					it = headRules.find("NP3");
					if( it!= headRules.end()) child=node->FindHeadChild(it->second);
				}
			}
			if(child==-1)
				child=0;
			//else
					//std::cout<<"rule': "<<it->first<<std::endl;

			if(node->GetSize()>child){
				//std::cout<<"Head': "<<node->GetNChild(child)->GetHead()<<std::endl;
				node->SetHead(node->GetNChild(child)->GetHead());
				node->SetHeadPOS(node->GetNChild(child)->GetHeadPOS());
			}
			//else
			//	std::cout<<"Head': no children "<<node->GetHead()<<std::endl;
		}
	}
}

void SyntaxTree::SetHeadOpenNodes(std::vector<SyntaxTreePtr > previousTrees){
	if(previousTrees.size()!=m_openNodes.size())
		std::cout<<"SetHeadOpenNodes: sizes don't match\n";
	else{
		for(int i=0;i<m_openNodes.size();i++){
			if(m_openNodes[i]->GetLabel().compare(previousTrees[i]->GetTop()->GetLabel())==0){
				//std::cout<<"Update head1: "<<m_openNodes[i]->GetHead();
				m_openNodes[i]->SetHead(previousTrees[i]->GetTop()->GetHead());
				m_openNodes[i]->SetHeadPOS(previousTrees[i]->GetTop()->GetHeadPOS());
				for(int j=0; j<previousTrees[i]->GetTop()->GetSize();j++){
					m_openNodes[i]->AddChild(previousTrees[i]->GetTop()->GetNChild(j));
				}
				if(previousTrees[i]->GetTop()->GetSize()==0)
					m_openNodes[i]->SetIsTerminal(true);
				//THIS IS WRONG!! -> memory leak at least
				//m_openNodes[i]=previousTrees[i]->GetTop();
				//std::cout<<"Update head12 "<<m_openNodes[i]->GetHead()<<std::endl;
			}
			else
				std::cout<<"SetHeadOpenNodes: labels don't match\n";
		}
	}
}

string* SyntaxTree::FindObj() const{
	//we might need to search for the entire subtree for the VP -> it may not be the first node
	//I should have a flag for when I reach the leaf of the rule so I don't recures to previous hypothesis -> something like HasHead
	// -> update the Node property with is leaf
	string *predArgPair = new string("");
	SyntaxNodePtr head;
	if(m_top->GetLabel().compare("VP")==0)
		head = m_top;
	else//!!!this is not OK -> we might have visited the VP in the previous hypothesis -> only need to serach in the current subtree
		//do the fake grammar and test
		head = m_top->FindChildRecursively("VP"); //!!!! This should be done recursively because the internal struct may have several layers

	if(head){
		//cout<<"VP: "<<m_top->GetHead()<<" NP: ";
		SyntaxNodePtr obj = head->FindFirstChild("NP"); //here I can add rules for finding to NPs (to deal with iobj)
		//uncomment to look for PPs
		if(!obj){
			obj = head->FindFirstChild("PP");
			//cout<<"PP: "<<obj->GetHead()<<endl;
		}
		if(obj){
			//cout<<obj->GetHead()<<endl;
			*predArgPair+=head->GetHead()+" "+obj->GetHead();
		}
	}
	return predArgPair;
}

string* SyntaxTree::FindSubj() const{
	//we might need to search for the entire subtree for the VP -> it may not be the first node
	//I should have a flag for when I reach the leaf of the rule so I don't recures to previous hypothesis -> something like HasHead
	// -> update the Node property with is leaf
	string *predArgPair = new string("");
	SyntaxNodePtr head;
	if(m_top->GetLabel().compare("S")==0)
		head = m_top;
	else//!!!this is not OK -> we might have visited the VP in the previous hypothesis -> only need to serach in the current subtree
		//do the fake grammar and test
		head = m_top->FindChildRecursively("S"); //!!!! This should be done recursively because the internal struct may have several layers

	if(head){
		//cout<<"VP: "<<m_top->GetHead()<<" NP: ";
		SyntaxNodePtr obj = head->FindFirstChild("NP");
		if(obj){
			//cout<<obj->GetHead()<<endl;
			*predArgPair+=head->GetHead()+" "+obj->GetHead();
		}
	}
	return predArgPair;
}

// END MARIA //

//!!!!! WHAT IS THIS ? - HOW DO I SPLIT THE STATES BASED ON THE INTERNAL STRUCTURE?? //
int SyntaxTreeState::Compare(const FFState& other) const
{
  const SyntaxTreeState &otherState = static_cast<const SyntaxTreeState&>(other);

  return 0;
}

////////////////////////////////////////////////////////////////
HeadFeature::HeadFeature(const std::string &line)
  :StatefulFeatureFunction(2, line) //should modify 0 to the number of scores my feature generates
	,m_headRules(new std::map<std::string, std::vector <std::string> > ())
	, m_probArg (new std::map<std::string, float> ())
	, m_lemmaMap (new std::map<std::string, std::string>())
	, m_allowedNT (new std::map<std::string, bool>())
	, m_counter(0)
	, m_counterDepRel(0)
	, m_cacheHits(0)
	, m_cacheDepRelHits(0)
{
  ReadParameters();
  //const char *vinit[] = {"S", "SQ", "SBARQ","SINV","SBAR","PRN","VP","WHPP","PRT","ADVP","WHADVP","XS"};//"PP", ??
  const char *vinit[] = {"S", "SQ", "SBARQ","SINV","SBAR"};//"PP", ??


	for(int i=0;i<sizeof(vinit)/sizeof(vinit[0]);i++){
		(*m_allowedNT)[vinit[i]]=true;
	}
  //m_headRules = new std::map<std::string, std::vector <std::string> > ();
  //m_probArg = new std::map<std::string, float> ();
}

void HeadFeature::ReadHeadRules(){
	std::ifstream file(m_headFile.c_str()); // (fileName);
	string line;
	std::vector<std::string> tokens;
	if(file.is_open()){
		while(getline(file,line)){
			split(line," \t",tokens);
			std::vector<std::string> tail (tokens.begin()+2,tokens.end());
			m_headRules->insert(std::pair<std::string, std::vector<std::string> > (tokens[1], tail));
			tokens.clear();
		}
	}
}

void HeadFeature::ReadProbArg(){
	std::ifstream file(m_probArgFile.c_str()); // (fileName);
	string line;
	string subcat="";
	std::vector<std::string> tokens;
	if(file.is_open()){
		while(getline(file,line)){
			// !!!! SPLIT doesn't work as it should -> problem with delimitors
			split(line," \t",tokens);
			subcat = tokens[0]+" "+tokens[1];
			m_probArg->insert(std::pair<std::string, float > (subcat, std::atof(tokens[2].c_str())));
			tokens.clear();
		}
	}
}

void HeadFeature::ReadLemmaMap(){
	std::ifstream file(m_lemmaFile.c_str()); // (fileName);
	string line,word,lemma;
	std::vector<std::string> tokens;
	if(file.is_open()){
		while(getline(file,line)){
			// !!!! SPLIT doesn't work as it should -> problem with delimitors
			split(line," \t",tokens);
			if(tokens.size()>1){
				m_lemmaMap->insert(std::pair<std::string, std::string > (tokens[0],tokens[1]));
			}
			tokens.clear();
		}
	}
}

void HeadFeature::SetParameter(const std::string& key, const std::string& value){
	  if(key=="rulesFile"){
		  m_headFile=value;
	  }
	  else {
	  	if(key=="probArgFile"){
	  		m_probArgFile=value;
	  	}
	  	else{
	  		if(key=="lemmaFile"){
	  			m_lemmaFile=value;
	  		}
	  		else{
	  			if(key=="jarPath"){
	  				m_jarPath=value;
	  			}
	  			else
	  				StatefulFeatureFunction::SetParameter(key, value);
	  		}
	  	}
	  }
  }

//NOT LOADED??
void HeadFeature::Load() {

  StaticData &staticData = StaticData::InstanceNonConst();
  staticData.SetHeadFeature(this);
  ReadHeadRules();
  ReadProbArg();
  ReadLemmaMap();

  //made CreateJavaVM a singleton class
  javaWrapper = CreateJavaVM::Instance(m_jarPath);
  //javaWrapper = new CreateJavaVM();
  //works when i first run this. must be a problem of loosing references to objects??


  //!!! running on the server: FATAL ERROR in native method: Bad global or local ref passed to JNI

  //this should be done per sentence
  //-> and if all sentences have access to the same object I need the called method to be synchronized in java
  //GetNewStanfordDepObj();
  //javaWrapper->GetDep("bllaa");
  // !!!! I NEED TO MAKE THIS CALL SO THE CALL IN EvaluateWhenApplied DOESN"T CRASH !!!!

  cerr<<"TEST CallStanfordDep:"<<endl;
  string temp = CallStanfordDep("(VP (VB give)(PP (DT a)(JJ separate)(NNP GC)(NN exam)))");
  cerr<<"TEMP DEP: "<<temp<<endl;

  //this should be done in InitializeForInput (for everysentence ->new thread)
  //the treadspecific pointer gets NULL when the thread exists


  //javaWrapper->TestRuntime();

}

std::string HeadFeature::CallStanfordDep(std::string parsedSentence) const{

	JNIEnv *env =  javaWrapper->GetAttachedJniEnvPointer();
	env->ExceptionDescribe();

	jobject rel = env->NewObject(javaWrapper->GetRelationsJClass(), javaWrapper->GetDepParsingInitJId());
			env->ExceptionDescribe();

	jobject workingStanforDepObj = env->NewGlobalRef(rel);
	env->DeleteLocalRef(rel);

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
		jstring jStanfordDep = reinterpret_cast <jstring> (env ->CallObjectMethod(workingStanforDepObj,javaWrapper->GetProcessParsedSentenceJId(),jSentence,jSpecified));

		env->ExceptionDescribe();

		//Returns a pointer to a UTF-8 string
		if(jStanfordDep != NULL){
		const char* stanfordDep = env->GetStringUTFChars(jStanfordDep, 0);
		std::string dependencies(stanfordDep);

		//how to make sure the memory gets released on the Java side?
		env->ReleaseStringUTFChars(jStanfordDep, stanfordDep);
		env->DeleteGlobalRef(workingStanforDepObj);
		env->ExceptionDescribe();
		javaWrapper->GetVM()->DetachCurrentThread();
		return dependencies;
		}
		//Something goes wrong in ProcessParsedSentence when trying to extract the relations and everythig crashes
		//returning a bogus string from ProcessParsedSentence works
		VERBOSE(1, "jStanfordDep is NULL" << std::endl);

		//env->DeleteLocalRef(jStanfordDep);

		//for some reson jStanfordDep is NULL -> maybe that the java thing crashses then DetachCurrentThread fails?
		if(workingStanforDepObj!=NULL){
			env->DeleteGlobalRef(workingStanforDepObj);
			env->ExceptionDescribe();
		}
		javaWrapper->GetVM()->DetachCurrentThread(); //-> when jStanfordDep in null it already crashed?

		return "null";
	}
	javaWrapper->GetVM()->DetachCurrentThread();


	return "exception";
}

//this should be called in the Manager or somewhere -> how?
//have to figure out how to manage this object
void HeadFeature::GetNewStanfordDepObj(){

		JNIEnv *env = javaWrapper->GetAttachedJniEnvPointer();

		//if an object was already initialized (for previous sentence) free the reference
		if(workingStanforDepObj)
			env->DeleteGlobalRef(workingStanforDepObj);

		//get the StanfordDep object to query
		jobject rel = env->NewObject(javaWrapper->GetRelationsJClass(), javaWrapper->GetDepParsingInitJId());
		env->ExceptionDescribe();
		//this reference remains valid until it is explicity freed -> prevents class from being unloaded
		//use this object and just call ProcessParsedSentence() with new arguments for each hypothesis
		workingStanforDepObj = env->NewGlobalRef(rel);
		env->DeleteLocalRef(rel);


		/**
		 * /Get list of selected dep relations -> Maybe print this at some point or just access the static object
		jobject relationsArray = env ->CallObjectMethod(relGlobal,javaWrapper->GetRelationListJId);
		env->ExceptionDescribe();
		cout<<"got relationsArray"<<endl;
		*/

		javaWrapper->GetVM()->DetachCurrentThread();

}

void HeadFeature::ProcessDepString(std::string depRelString, std::vector< SyntaxTreePtr > previousTrees,ScoreComponentCollection* accumulator) const{
	vector<string> tokens;
	split(depRelString,"\t",tokens);
	std::map<std::string, bool> seenDepRel;
	if(tokens.size()>1){
		cerr<<tokens.size()<<endl;
		for(vector<SyntaxTreePtr>::iterator itTrees=previousTrees.begin();itTrees!=previousTrees.end();itTrees++)
			seenDepRel.insert((*itTrees)->m_seenDepRel.begin(),(*itTrees)->m_seenDepRel.end());
		for(vector<string>::iterator it=tokens.begin();it!=tokens.end();it++){
				if(seenDepRel.find(*it)==seenDepRel.end()){
					//score
					std::map<string,float>::iterator itScore;
					itScore = m_probArg->find(*it);
					if(itScore!=m_probArg->end()){
						//cout<<"Have value: "<<it->second<<endl;
						vector<float> scores;
						scores.push_back(log(itScore->second+0.001));
						scores.push_back(1.0);
						//accumulator->PlusEquals(this,log(it->second+0.001));
						accumulator->PlusEquals(this,scores);
					}
				//add to hashmap
				seenDepRel[*it]=true;
				}
		}
		//m_lemmaMap->insert(std::pair<std::string, std::string > (tokens[0],tokens[1]));
	}
}

FFState* HeadFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {

			//basically at the start of a new sentence reset counter
		//use this just for testing with one thread
			if(cur_hypo.GetId()==0){
				std::cerr<<"Current counter: "<<m_counter<<endl;
				std::cerr<<"Current counterDepRel: "<<m_counterDepRel<<endl;
				std::cerr<<"Current cacheHits: "<<m_cacheHits<<endl;
				std::cerr<<"Current cacheDepRelHits: "<<m_cacheDepRelHits<<endl;
				StringHashMap &localCache = GetCache();
				DepRelMap &localCacheDepRel = GetCacheDepRel();
				//nr of S or VP trees = cache.size + cache.hits
				std::cerr<<"Current cache size: "<<localCache.size()<<endl;
				// Counter = cacheDepRel.size + cacheDepRel.hits
				std::cerr<<"Current cacheDepRel size: "<<localCacheDepRel.size()<<endl;
				localCache = ResetCache();
				localCacheDepRel = ResetCacheDepRel();
				std::cerr<<"Reset cache: "<<localCache.size()<<endl;
				std::cerr<<"Reset cacheDepRel: "<<localCacheDepRel.size()<<endl;
				m_counter=0;
				m_counterDepRel=0;
				m_cacheHits=0;
				m_cacheDepRelHits =0;

			}


	    const std::string *tree = property->GetValueString();

	    SyntaxTreePtr syntaxTree (new SyntaxTree());
	    //should have new SyntaxTree(pointer to headRules)
	    syntaxTree->FromString(*tree,m_lemmaMap);
	    //std::cout<<*tree<<std::endl;

	    //get subtrees (in target order)
	        std::vector< SyntaxTreePtr > previousTrees;
	        for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
	          const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
	          if (word.IsNonTerminal()) {
	            size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
	            /*
	              * Notes: When evaluating the value of this feature function, you should avoid
	              * calling hypo.GetPrevHypo().  If you need something from the "previous"
	              * hypothesis, you should store it in an FFState object which will be passed
	              * in as prev_state.  If you don't do this, you will get in trouble.
	             */
	            const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
	            //trebuie cumva pt prev hypothesis sa am si un container the <node*,node*> pt dep pairs already scored/seen
	            const SyntaxTreeState* prev = dynamic_cast<const SyntaxTreeState*>(prevHypo->GetFFState(featureID));
	            //SyntaxTree* prev_tree = prev->GetTree();
	            previousTrees.push_back(prev->GetTree());
	            //here add previousScoredPairs -> the FFstate should have a pointer to the container
	          }
	        }

	        syntaxTree->SetHeadOpenNodes(previousTrees);
	        syntaxTree->FindHeads(syntaxTree->GetTop(), *m_headRules);

	        /*
	        std::cout<<*tree<<std::endl;
	        std::cout<< "rule: "<< syntaxTree->ToString()<<std::endl;
	   	    std::cout<< "rule maxLevel: "<< syntaxTree->ToStringLevel(2)<<std::endl;
     	    std::cout<< "rule maxNodes: "<< syntaxTree->ToStringNodeCount(3)<<std::endl;
					*/

	        /*
	        std::stringstream *subtree = new stringstream("");
	        syntaxTree->ToStringDynamic(syntaxTree->GetTop(),&previousTrees,subtree);
	        std::string parsedSentence = subtree->str();
	        syntaxTree->SetRootedTree(parsedSentence);
	        delete subtree;
					*/

	        std::string depRel ="";
	        //should only call toString if the LHS passes these criteria
	        if(m_allowedNT->find(syntaxTree->GetTop()->GetLabel())!=m_allowedNT->end()){
	        	//std::string parsedSentence  = syntaxTree->ToString();
	        	std::string parsedSentence  = syntaxTree->ToStringLevel(4);
	        	if(parsedSentence.find_first_of("Q")==string::npos){// && parsedSentence.find("VP")==1){ //if there is no Q in the subtree (no glue rule applied)
	        		//I should populate this cache with all trees constructed? and just set to "" if I haven't extracted the depRel?
	        		StringHashMap &localCache = GetCache();
	        		if(localCache.find(parsedSentence)!=localCache.end()){
	        			depRel=localCache[parsedSentence]->first;
	        			m_cacheHits++;
	        			//cerr<<"cache Hit: "<<parsedSentence <<endl;
	        			//cerr<<"dep rel: "<<depRel<<endl;
	        		}
	        		else{
	        			depRel = CallStanfordDep(parsedSentence); //(parsedSentence);
	        			float score = 1.0;
	        			DepRelMap &localCacheDepRel = GetCacheDepRel();
	        			//if key already returns return iterator to key position
	        			pair<DepRelMap::iterator,bool> it = localCacheDepRel.insert(pair<string, float> (depRel,score));
	        			if(it.second==false) //no insert took place
	        				m_cacheDepRelHits++;
	        			if(it.first!=localCacheDepRel.end())
	        				(*m_cache)[parsedSentence]=it.first;
	        			//save in TreeString - DepRel cache
	        			//parsedSentence should be produced with generic words since the rel doesn't depend on the words
	        			//therefore we can cache only repeating trees ignoring the words and avoid more computation
	        			//FALSE some words matter -> copula verb, timeregex etc. Nouns probably don't matter, finite verbs, adj etc
	        			//we need the preposition to make prep_to rep rels or substitute in post_processing
	        			//I could also precompute the relations for the most common trees of up to some size that I've seen in the corpus
	        			//especially if I put a treshold level on the hight
	        			//!!! might want to hald the size of the cache from time to time
	        			//-> do like the phrasse chache: memorize the time and delete older ones (lower in the chart)
	        			//instead of time I could also just save the span length -> if the number of leavs are different so will the tree
	        			//(*m_cache)[parsedSentence]=depRel;

	        			//std::cerr<<"Cache Miss: "<<parsedSentence<<endl;
	        			//std::cerr<< "dep rel: "<<depRel<<endl;
	        		}
							if(depRel!=" "){
								m_counter++;
								vector<string> tokens;
								Tokenize(tokens,depRel,"\t");
								//split(depRel,"\t",tokens);
								m_counterDepRel+=tokens.size();


								//std::cerr<< "token size: "<<tokens.size()<<endl;
								//ProcessDepString(depRel,previousTrees,accumulator);
							}
							//problem when there is no dep rel ? returns '\0' or NULL
							// FUCK THIS FUCKING ERRORS IT FAILS EVEN WITH: extended rule: (VP (VB give)(PP (DT a)(JJ separate)(NNP GC)(NN exam)))
							//javaWrapper->GetDep(parsedSentence);
							//this fails here but not in Load()!!!
							//javaWrapper = new CreateJavaVM();
							//javaWrapper->TestRuntime();
	        	}
	        }
	        //std::cout<< "dep rel: "<< stanfordDep<<std::endl;

	        int index = 0;

	        string *predArgPair = syntaxTree->FindObj();
	        std::map<string,float>::iterator it;
	        //!!! SOULD TRY TO CACHE IT -> THEN I NEED SYNC FOR MULTITHREAD !!!
	        //I should only search if predArgPair is not empty
	        if(*predArgPair!=""){
	        	//cout<<"Found pair: "<<*predArgPair<<endl;
	        	it = m_probArg->find(*predArgPair);
						if(it!=m_probArg->end()){
							//cout<<"Have value: "<<it->second<<endl;
							vector<float> scores;
							scores.push_back(log(it->second+0.001));
							scores.push_back(1.0);
							//accumulator->PlusEquals(this,log(it->second+0.001));
							accumulator->PlusEquals(this,scores);
						}

						//else !! I should do smoothing based on the verb
					 //	accumulator->PlusEquals(this,-1);
					//maybe I should just train a bigram lm over pred-arg pairs and query it here that takes care of smoothing -> dep lm for obj
						//probability of vb having n arguments and which types -> when I reach the VP I should already have all the arguments covered
						//prob of vb with argument X given X is aligned to source word attached to verb Y
	        }

	        delete predArgPair;
	        predArgPair = 0;
/*
	        string *predSubjPair = syntaxTree->FindSubj();
	        	        if(*predSubjPair!=""){
	        	        		cout<<"Found pair: "<<*predSubjPair<<endl;
	        	        }
*/
	        //std::cout<< syntaxTree->ToStringHead()<<std::endl;

	        return new SyntaxTreeState(syntaxTree);



	}
	vector<float> scores;
	scores.push_back(1000.0);
	scores.push_back(0.0);
	accumulator->PlusEquals(this,scores);
}



}

