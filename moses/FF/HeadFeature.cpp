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

using namespace std;

namespace Moses
{

// MARIA //

void split(std::string &s, std::string delim, std::vector<std::string> &tokens) {
	boost::trim(s);
	const std::locale& Loc =std::locale();
	boost::trim_if(s,::boost::is_space(Loc));
	boost::find_format_all(s,
			boost::token_finder(::boost::is_space(Loc), boost::token_compress_on),
			 boost::const_formatter(boost::as_literal(" ")));
	//boost::trim_fill(s," ");//trim_all(s," "); //trim_if(s, boost::is_any_of(" \t\0\n"));
    boost::split(tokens, s, boost::is_any_of(delim));
//    cout<<s<<" "<<tokens.size()<<endl;
}

SyntaxNode* SyntaxNode::FindFirstChild(std::string label) const{
	for(int i=0; i<m_children.size();i++){
		if(m_children[i]->m_label.compare(label)==0)
				return m_children[i];
	}
	return NULL;
}

int SyntaxNode::FindHeadChild(std::vector<std::string> headRule){
	int i,j;
	int found=-1;
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
			for(j=1; j<headRule.size();j++) //first item is direction
				if(m_children[i]->m_label.compare(headRule[j])==0){
					found=i;
					break;
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
					found=i;
					break;
				}
		}
	}
	return found;
}

SyntaxTree::~SyntaxTree()
{
  Clear();
}

void SyntaxTree::Clear()
{
  m_top = 0;
  // loop through all m_nodes, delete them
  for(size_t i=0; i<m_nodes.size(); i++) {
    delete m_nodes[i];
  }
  m_nodes.clear();

}



SyntaxNode* SyntaxTree::AddNode( int startPos, int endPos, std::string label )
{
  SyntaxNode* newNode = new SyntaxNode( startPos, endPos, label );
  m_nodes.push_back( newNode );
  m_size ++;// std::max(endPos+1, m_size);
  return newNode;
}

void SyntaxTree::AddNode( SyntaxNode *newNode)
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



SyntaxNode* SyntaxTree::FromString(std::string internalTree)
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
			SyntaxNode* newNode;
			nextpos = internalTree.find_first_of("][",pos);
			nodes.clear();
			std::string temp = internalTree.substr(pos,nextpos-pos);
			split(temp,"\t ",nodes);
			if(nodes.size()==0)
				std::cerr<<" syntax string is not well formed: empty node"<<std::endl;
			if(internalTree[nextpos]==']'){
				newNode = new SyntaxNode( 0, 0, nodes[0]);
				//can have [TOP [S] ] pr [NP [PRP I]]
				if(nodes.size()==2){
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
				newNode = new SyntaxNode( 0, 0, nodes[0]);
				AddNode(newNode);
			}
			m_attachTo = newNode;
			pos = nextpos;
		}
		if(token == ']')
			m_attachTo = m_attachTo->GetParent();
	}

	return m_top;
}

void SyntaxTree::ToString(SyntaxNode *node, std::stringstream &tree){
	tree <<"["<< node->GetLabel()<< " ";
	if(node->IsTerminal())
		tree << node->GetHead() << "]";
	else{
		for(int i=0;i<node->GetSize();i++)
			ToString(node->GetNChild(i),tree);
		tree << "]";
	}

}

void SyntaxTree::ToStringHead(SyntaxNode *node, std::stringstream &tree){
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
void SyntaxTree::FindHeads(SyntaxNode *node, std::map<std::string, std::vector <std::string> > &headRules) const {
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

void SyntaxTree::SetHeadOpenNodes(std::vector<SyntaxTree*> previousTrees){
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
	if(m_top->GetLabel().compare("VP")==0){
		//cout<<"VP: "<<m_top->GetHead()<<" NP: ";
		SyntaxNode *obj = m_top->FindFirstChild("NP");
		if(obj){
			//cout<<obj->GetHead()<<endl;
			*predArgPair+=m_top->GetHead()+" "+obj->GetHead();
		}
	}
	return predArgPair;
}

// END MARIA //


int SyntaxTreeState::Compare(const FFState& other) const
{
  const SyntaxTreeState &otherState = static_cast<const SyntaxTreeState&>(other);

  return 0;
}

////////////////////////////////////////////////////////////////
HeadFeature::HeadFeature(const std::string &line)
  :StatefulFeatureFunction(1, line) //should modify 0 to the number of scores my feature generates
{
  ReadParameters();
  m_headRules = new std::map<std::string, std::vector <std::string> > ();
  m_probArg = new std::map<std::string, float> ();
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

void HeadFeature::SetParameter(const std::string& key, const std::string& value){
	  if(key=="rulesFile"){
		  m_headFile=value;
	  }
	  else {
	  	if(key=="probArgFile"){
	  	m_probArgFile=value;
	  	}
	  	else
	      StatefulFeatureFunction::SetParameter(key, value);
	    }
  }

//NOT LOADED??
void HeadFeature::Load() {

  StaticData &staticData = StaticData::InstanceNonConst();
  staticData.SetHeadFeature(this);
  ReadHeadRules();
  ReadProbArg();
}


FFState* HeadFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {
	    const std::string *tree = property->GetValueString();



	    SyntaxTree *syntaxTree = new SyntaxTree();
	    //should have new SyntaxTree(pointer to headRules)
	    syntaxTree->FromString(*tree);
	    //std::cout<<*tree<<std::endl;
	    //std::cout<< syntaxTree->ToString()<<std::endl;//" size "<<syntaxTree->GetSize()<< " open "<<syntaxTree->GetOpenNodes().size()<<std::endl<<std::endl;



	    //get subtrees (in target order)
	        std::vector<SyntaxTree*> previousTrees;
	        for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
	          const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
	          if (word.IsNonTerminal()) {
	            size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
	            const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
	            const SyntaxTreeState* prev = dynamic_cast<const SyntaxTreeState*>(prevHypo->GetFFState(featureID));
	            SyntaxTree* prev_tree = prev->GetTree();
	            previousTrees.push_back(prev_tree);
	          }
	        }

	        syntaxTree->SetHeadOpenNodes(previousTrees);

	        int index = 0;
	        syntaxTree->FindHeads(syntaxTree->GetTop(), *m_headRules);
	        string *predArgPair = syntaxTree->FindObj();
	        //cout<<"Found pair: "<<*predArgPair<<endl;
	        std::map<string,float>::iterator it;
	        it = m_probArg->find(*predArgPair);
	        if(it!=m_probArg->end()){
	        	//cout<<"Have value: "<<it->second<<endl;
	        	accumulator->PlusEquals(this,log(it->second+0.01));
	        }
	        else
	        	accumulator->PlusEquals(this,-1);


	        //std::cout<< syntaxTree->ToStringHead()<<std::endl;

	        return new SyntaxTreeState(syntaxTree);


	}

	accumulator->PlusEquals(this,1000);
}



}

