#include "SelPrefFeature.h"

#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/StaticData.h"
#include "moses/ChartHypothesis.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/TreeStructurePhraseProperty.h"
#include "InternalTree.h"

#include <string>
#include <unordered_map>
#include <set>
#include <queue>
#include <boost/algorithm/string.hpp>

using namespace std;


namespace Moses
{

SelPrefFeature::SelPrefFeature(const std::string &line)
  :StatefulFeatureFunction(0, line){

	ReadParameters();
}

void SelPrefFeature::SetParameter(const std::string& key, const std::string& value){

}

void SelPrefFeature::Load() {

  StaticData &staticData = StaticData::InstanceNonConst();
  //staticData.SelPrefFeature(this);
}

// For clearing caches and counters

void SelPrefFeature::CleanUpAfterSentenceProcessing(const InputType& source){
}

/*
 * For all internal nodes of a tree, replace binarized nodes (e.g. ^NP) with its children.
 * Example:
 * Binarized: [prep [det [DT the2]][^IN [IN of] [^pobj [pobj [det [DT the]] [NN session2]]]]]
 * Unbinarized: [prep [det [DT the2]] [IN of] [pobj [det [DT the]] [NN session2]]]
 */
void UnbinarizeTree(TreePointer mytree){
	vector<TreePointer> &currentChildren = mytree->GetChildren();
	vector<TreePointer> unbinarizedChildren;
	for (auto &child : currentChildren){
		UnbinarizeTree(child);
		if(!child->IsLeafNT() && child->GetLabel().GetString(0).as_string()[0] == '^'){
			for (auto &grandchild: child->GetChildren()){
				unbinarizedChildren.push_back(grandchild);
			}
		}
		else
			unbinarizedChildren.push_back(child);
	}
	currentChildren = unbinarizedChildren;
}

/*
* Recursively find head words for nodes in the internal tree
*  arg1: TreePointer tree -> current node
*  arg2: vector<TreePointer> &previous_trees -> pointers to the root nodes of previous hypothesis
*  arg3: vector<Word> &childrenHeadWords -> head words for all non-terminals in the internal tree (except for pre-terminals)
*  arg4: size_t &child_id ->  keeps track of the leafNTs in order to access the corresponding hypothesis in previous_trees vector
*
* For dependency representation each node has a pre-terminal child.
* The head of the node is the terminal corresponding to this pre-terminal child.
* If there are several pre-terminal children, use the first one.
* Recursively find the head words for all non-terminals in the internal tree (except for pre-terminals)
* Return a vector of head words for the internal tree.
*
* vector childreadHeadWords will save all the head words in the tree, in pre-order traversal of the corresponding non-terminals (that are not pre-terminals)
* pre-order traversal -> root [left_child, right_child]
* Example: [prep [IN of] [pobj [det [DT the]] [NN session]]]
* 			pre-order traversal on NTs: pobj det
*			childreanHeadWords = {session, the}
*/
bool SelPrefFeature::FindHeadRecursively(
		TreePointer tree, const std::vector<TreePointer> &previous_trees,
		const std::vector<HeadsPointer> &previous_heads,
		std::unordered_map<InternalTree*, const Word*> &childrenHeadWords, size_t &childId) const{
	TreePointer headTree = nullptr;
	bool found = false;
	for (auto child : tree->GetChildren()){
		// if there is a leaf NT in the current internal tree of the rule,
		// then look in the previous trees for the corresponding node (and search there for the head)
		if(child->IsLeafNT()){
			headTree = previous_trees[childId];
			if(headTree->GetLength() == 1 && headTree->GetChildren()[0]->IsTerminal()){
				if(!found){
					childrenHeadWords[tree.get()] = &headTree->GetChildren()[0]->GetLabel();
					found = true;
				}
			}
			else{
				const auto head = previous_heads[childId]->find(headTree.get());//get the head of that subtree
				if (head!=previous_heads[childId]->end()){
					//empty string -> if a non-terminal has no pre-terminal child. e.g. for glue rules
					childrenHeadWords[child.get()] = head->second;
				}

			}
			childId++;
			continue; //don't recurse in the previous hypothesis
		}
		else
			headTree = child;
		// extract the head from the **first** pre-terminal child
		// a node is a pre-terminal (POS label) if it has just one child and that child is a terminal node
		if(headTree->GetLength() == 1 && headTree->GetChildren()[0]->IsTerminal()){
			// the head of the subtree is the terminal corresponding to the first pre-terminal child
			if(!found){
				childrenHeadWords[tree.get()] = &headTree->GetChildren()[0]->GetLabel();
				found = true;
			}
		}
		else {
			// recursively find heads for internal tree (for non-terminal children of the current node that are not pre-terminals)
			// Example: [prep [IN of] [pobj [det [DT the]] [NN session]]] -> head of pobj
			if (!headTree->IsTerminal()) // should never be the case we recursively reached a terminal
				FindHeadRecursively(headTree, previous_trees, previous_heads, childrenHeadWords, childId);
		}

	}
	return found;
}

TreePointer FindChildByLabel(TreePointer tree, string label){
	for(auto child : tree->GetChildren()){
		string rel = child->GetLabel().ToString();
		boost::trim(rel);
		if (rel == label)
			return child;
	}
}

/*
 * Returns tuples of parent-head-word child-head-word relation, if the child relation belongs to: nsubj, nsubjpass, dobj, iobj, prep
 * Iterates through all the nodes in the current hypothesis.
 * For prep child relation have to look for the head word of the pobj child
 * perhaps should consider: prepc: prepositional clausal modifier => “He purchased it without paying a premium” prepc_without(purchased, paying)
*/
void SelPrefFeature::MakeTuples(
		TreePointer tree,
		std::unordered_map<InternalTree*, const Word*> &childrenHeadWords,
		vector<vector<string>> &depRelTuples) const{
	set<string> allowedLabels = {"nsubj","nsbujpass", "dobj", "iobj", "prep"};
	queue<TreePointer> BFSQueue;
	BFSQueue.push(tree);
	while(!BFSQueue.empty()){
		TreePointer currentNode = BFSQueue.front();
		BFSQueue.pop();
		for (auto &child : currentNode->GetChildren()){
			// use GetString(factor 0) instead of ToString
			string rel = child->GetLabel().ToString();
			boost::trim(rel);
			if(!child->IsTerminal() && !child->IsLeafNT()
					&& allowedLabels.find(rel) != allowedLabels.end()){
				// {rel, head, dep}
				if (rel == "prep"){
					rel = rel + "_" + childrenHeadWords.find(child.get())->second->ToString();
					child = FindChildByLabel(child, "pobj");
				}
				string dep = childrenHeadWords.find(child.get())->second->ToString();
				boost::trim(dep);
				string head = childrenHeadWords.find(currentNode.get())->second->ToString();
				boost::trim(head);
				vector<string> tuple = {rel, head, dep};
				depRelTuples.push_back(tuple);
			}
			BFSQueue.push(child);
		}
	}
}


FFState* SelPrefFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{

	if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {
		const std::string *tree = property->GetValueString();
		TreePointer mytree (boost::make_shared<InternalTree>(*tree));
		// get pointers to the root of previous hypothesis, in target order of the leaf NTs in the current hypothesis
		std::vector<TreePointer> previous_trees;
		std::vector<HeadsPointer> previous_heads;
		for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
		  const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
		  if (word.IsNonTerminal()) {
			size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
			const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
			const SelPrefState* prev = static_cast<const SelPrefState*>(prevHypo->GetFFState(featureID));
			const TreePointer prev_tree = prev->GetTree();
			previous_trees.push_back(prev_tree);
			const HeadsPointer prev_head = prev->GetHeads();
			previous_heads.push_back(prev_head);

		  }
		}

		UnbinarizeTree(mytree);

		// find head word for the root of the current hypothesis and the head words for the internal non-terminals
		std::unordered_map<InternalTree*, const Word*> childrenHeadWords;
		size_t childId = 0;
		FindHeadRecursively(mytree, previous_trees, previous_heads, childrenHeadWords, childId);

		vector<vector<string>> depRelTuples;
		MakeTuples(mytree, childrenHeadWords, depRelTuples);

		// combine with previous hypothesis -> extend leafNTs to point to TreePointers of prevHyp
		// Why do I need this?
		mytree->Combine(previous_trees);

		return new SelPrefState(mytree,make_shared<std::unordered_map<InternalTree*,const Word*>> (childrenHeadWords));
	}
	else {
	    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
	}

}


}
