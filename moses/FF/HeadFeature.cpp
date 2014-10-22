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
					if(head!=""){
						newNode->SetHead(head);
					}
					else
						newNode->SetHead(nodes[1]);
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
	tree <<"["<< node->GetLabel()<< " ";
	if(node->IsTerminal())
		tree << node->GetHead() << "]";
	else{
		for(int i=0;i<node->GetSize();i++)
			ToString(node->GetNChild(i),tree);
		tree << "]";
	}

}

void SyntaxTree::ToStringHead(SyntaxNodePtr node, std::stringstream &tree){
	tree <<"["<< node->GetLabel()<<"-"<<node->GetHead()<< " ";
	if(node->IsTerminal())
		tree << node->GetHead() << "]";
	else{
		for(int i=0;i<node->GetSize();i++)
			ToStringHead(node->GetNChild(i),tree);
		tree << "]";
	}

}

std::string SyntaxTree::ToString(){
	std::stringstream tree("[");
	ToString(m_top,tree);
	return tree.str();
}

std::string SyntaxTree::ToStringHead(){
	std::stringstream tree("[");
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
		}
		else{
			if(node->GetSize()>0){
				//std::cout<<"Head: "<<node->GetNChild(0)->GetHead()<<std::endl;
				node->SetHead(node->GetNChild(0)->GetHead());
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
		SyntaxNodePtr obj = head->FindFirstChild("NP");
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
	, m_lemmaMap (new std::map<std::string, std::string>)
{
  ReadParameters();
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
	  		else
	  			StatefulFeatureFunction::SetParameter(key, value);
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
}


FFState* HeadFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {
	    const std::string *tree = property->GetValueString();



	    SyntaxTreePtr syntaxTree (new SyntaxTree());
	    //should have new SyntaxTree(pointer to headRules)
	    syntaxTree->FromString(*tree,m_lemmaMap);
	    //std::cout<<*tree<<std::endl;
	   //std::cout<< syntaxTree->ToString()<<std::endl;



	    //get subtrees (in target order)
	        std::vector< SyntaxTreePtr > previousTrees;
	        for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
	          const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
	          if (word.IsNonTerminal()) {
	            size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
	            const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
	            const SyntaxTreeState* prev = dynamic_cast<const SyntaxTreeState*>(prevHypo->GetFFState(featureID));
	            //SyntaxTree* prev_tree = prev->GetTree();
	            previousTrees.push_back(prev->GetTree());
	          }
	        }

	        syntaxTree->SetHeadOpenNodes(previousTrees);

	        int index = 0;
	        syntaxTree->FindHeads(syntaxTree->GetTop(), *m_headRules);
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

	        //std::cout<< syntaxTree->ToStringHead()<<std::endl;

	        return new SyntaxTreeState(syntaxTree);


	}
	vector<float> scores;
	scores.push_back(1000.0);
	scores.push_back(0.0);
	accumulator->PlusEquals(this,scores);
}



}

