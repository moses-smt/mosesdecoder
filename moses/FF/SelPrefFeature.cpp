#include "SelPrefFeature.h"

#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/StaticData.h"
#include "moses/ChartHypothesis.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/TreeStructurePhraseProperty.h"
#include "InternalTree.h"
#include "moses/Util.h"

#include <string>
#include <unordered_map>
#include <queue>
#include <stack>
#include <boost/algorithm/string.hpp>
#include <fstream>

#include <boost/regex.hpp>

using namespace std;



namespace Moses
{

SelPrefFeature::SelPrefFeature(const std::string &line)
  : StatefulFeatureFunction(1, line)
	, m_modelFileARPA("")
	, m_MIModelFile("")
	, m_lemmaFile("")
	, m_WBmodel(nullptr)
	, m_MIModel(nullptr)
	// allowing ^nsubj because it appears at unbinarized leafNT could get you to score nsubj -> ^nsubj a relation
	// !!BAD -> ^nsubj tokyo Sich is not known by the model
	// -> need to go to prev hypothesis in Make Tuples and search the children then mark them as seen -> substract previous score and add new one
	/*
	 * nsubj meet tokyo
SCORE: -6.018 -19299.309
^nsubj tokyo Sich
SCORE: -5.016 -19304.324
[^root [dep [VBD] [nsubj [NNP] [^nsubj]]] [punct [. .]]]
[^root [dep [VBD] [nsubj [NNP] [^nsubj]]] [punct [. .]]]
[^root [dep [VBD met] [nsubj [NNP Tokyo] [^nsubj [NN sich] [prep [IN into] [pobj [det [DT the]]...
	 */
	, m_allowedLabels{"nsubj","nsbujpass", "dobj", "iobj", "prep"}//,"^nsubj","^nsbujpass", "^dobj", "^iobj", "^prep"}
	, m_unbinarize(false)
	, m_lemmaMap(nullptr)
   {

	ReadParameters();
   }

void SelPrefFeature::SetParameter(const std::string& key, const std::string& value){
	if(key=="modelFileARPA"){
		m_modelFileARPA = value;
	} else if(key == "lemmaFile"){
		m_lemmaFile = value;
	} else if (key == "unbinarize"){
		if (value == "true")
			m_unbinarize = true;
	} else if(key=="MIModelFile"){
			m_MIModelFile = value;
	}

/*	else{
		StatefulFeatureFunction::SetParameter(key,value);
	}
*/

}

void split(std::string &s, std::string delim, std::vector<std::string> &tokens) {
	const std::locale& Loc =std::locale();
	boost::trim_if(s,::boost::is_space(Loc));
	boost::find_format_all(s,
			boost::token_finder(::boost::is_space(Loc), boost::token_compress_on),
			 boost::const_formatter(boost::as_literal(" ")));
    boost::split(tokens, s, boost::is_any_of(delim));
}


void SelPrefFeature::ReadLemmaMap(){
	shared_ptr<unordered_map<string, string>> (new unordered_map<string, string>).swap(m_lemmaMap);
	if(!m_lemmaMap)
		cerr << "Error creating lemma map" << endl;
	else
		cerr << "Loading lemma map" << endl;
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

void SelPrefFeature::ReadMIModel(){
	shared_ptr<map<vector<string>, vector<float>>> (new map<vector<string>, vector<float>>).swap(m_MIModel);
	std::ifstream file(m_MIModelFile.c_str()); // (fileName);
	string line;
	vector<string> tokens;
	vector<float> scores;
	if(file.is_open()){
		while(getline(file,line)){
			// !!!! SPLIT doesn't work as it should -> problem with delimitors
			split(line," \t",tokens);
			// rel head dep
			vector<string> subcat = {tokens[0], tokens[1], tokens[2]};
			scores.push_back(std::atof(tokens[3].c_str()));
			scores.push_back(std::atof(tokens[4].c_str()));
			m_MIModel->insert(pair<vector<string>, vector<float>> (subcat,scores));
			tokens.clear();
			scores.clear();
		}
	}
}

std::string FilterArg(std::string arg, shared_ptr< std::unordered_map<std::string, std::string>> lemmaMap){
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
	auto it = lemmaMap->find(arg);
	if(it!=lemmaMap->end()){
		return it->second;
	}
	else
		return arg; //no filter needed, use the original arg

}


void SelPrefFeature::Load() {

  StaticData &staticData = StaticData::InstanceNonConst();
  //staticData.SelPrefFeature(this);

  if(m_modelFileARPA != ""){
	 shared_ptr<lm::ngram::Model> (new lm::ngram::Model(m_modelFileARPA.c_str())).swap(m_WBmodel);
  }

  if(m_lemmaFile != ""){
	  ReadLemmaMap();
  }

  if(m_MIModelFile != ""){
	  ReadMIModel();
  }
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
// todo: perhaps combine unbinarize with FindHead and MakeTuple
// -> when the recursive call returns and we have access to the unbinarized children (inclusive ^leafNT)
void UnbinarizeTree(TreePointer mytree){
	vector<TreePointer> &currentChildren = mytree->GetChildren();
	vector<TreePointer> unbinarizedChildren;
	for (auto &child : currentChildren){
		// !!! PROBLEM -> nead a IsLeafNT relative to the current hypothesis.
		// If I apply combine before unbinarize, head search and make tuples than IsLeafNT() doesn't work for leafNTs of the currenty hyp
		// And then the functions recurse through all previous hypothesis
		// If I don't combine than I need the vector of previous trees
		//if(!child->IsLeafNT())
			UnbinarizeTree(child);

		// !!maybe I should call FindHead and MakeTuples here!!

		// problem with leafNT that are not unbinarized -> should recurse to previous hypothesis
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

TreePointer FindChildByLabel(TreePointer tree, string label){
	for(auto child : tree->GetChildren()){
		string rel = child->GetLabel().ToString();
		boost::trim(rel);
		if (rel == label)
			return child;
	}
	return nullptr;
}

inline string ToString(const Word *word){
	return word->GetFactor(0)->GetString().as_string();
}


void PrintHeadWords(TreePointer tree){
	stack< TreePointer> DFSStack;
	DFSStack.push(tree);
	cout << "Heads: ";
	while(!DFSStack.empty()){
		auto currentNode = DFSStack.top();
		DFSStack.pop();
		if(currentNode->GetHead().get() != nullptr)
			cout << currentNode->GetLabel().ToString() << " " << currentNode->GetHead()->ToString() << " ";
		if(currentNode->IsLeafNT())
			continue;
		for (auto child : currentNode->GetChildren()){
			DFSStack.push(child);
		}
	}
	cout << endl;

}

/*
* Recursively find head words for nodes in the internal tree
*  arg1: TreePointer tree -> current node
*  arg2: vector<TreePointer> &previous_trees -> pointers to the root nodes of previous hypothesis
*  arg3: size_t &child_id ->  keeps track of the leafNTs in order to access the corresponding hypothesis in previous_trees vector
*
* For dependency representation each node has a pre-terminal child.
* The head of the node is the terminal corresponding to this pre-terminal child.
* If there are several pre-terminal children, use the first one.
* Recursively find the head words for all non-terminals in the internal tree (except for pre-terminals)
* If we have leaf virtual nodes (^leafNT) recurse to previous hypothesis to retrieve the head
* Root virtual nodes are a problem for scoring
*  -> the score involving that root virtual node should be subtracted when extending the hypothesis higher in the chart
* Extend treepointer with headword information
*
*/
// todo: !!! solve recursion in previous trees for fininding heads via ^leafNT -> check Rico's emnlp2014 paper
bool SelPrefFeature::FindHeadRecursively(
		TreePointer tree, const std::vector<TreePointer> &previous_trees,
		size_t &childId) const{
	TreePointer headTree = nullptr;
	bool found = false;
	for (auto child : tree->GetChildren()){
		// if there is a leaf NT in the current internal tree of the rule,
		// then look in the previous trees for the corresponding node (and search there for the head)
		if(child->IsLeafNT()){
			headTree = previous_trees[childId];
			// it shouldn't be the case that a pre-terminal is a privous hypothesis (unary rule)
			// prep_of in tokyo
			// SCORE: -6.280 -12551.850
			// [^root [VBN] [^root [dep [IN] [prep]] [punct [. .]]]]
			// [^root [VBN] [dep [IN] [prep]] [punct [. .]]]
			// [^root [VBN gathered] [dep [IN in] [prep [IN of] [pobj [NNP Tokyo]]]] [punct [. .]]]
			if(headTree->GetLength() == 1 && headTree->GetChildren()[0]->IsTerminal()){
				if(!found){
					tree->SetHead(make_shared<Word> (headTree->GetChildren()[0]->GetLabel()));
					found = true;
				}
			}
			else{

		/*		Expand virtual nodes by retrieving children from previous hypothesis. Then search for the head node
		 * 		if(headTree->GetLabel().GetString(0).as_string()[0] == '^'){
					// should solve: nsubj -> ^nsubj -> Minister
					// [^root [nsubj [det [DT the]] [^nsubj]] [VBZ has] [dobj] [^root]]
					// [^root [nsubj [det [DT the]] [^nsubj [nn [NNP Prime]] [NNP Minister]]] [VBZ has] [dobj [DT all]
					for (auto grandchild :  headTree->GetChildren()) {
						if(grandchild->GetLength() == 1 && grandchild->GetChildren()[0]->IsTerminal()){
							if(!found){
								tree->SetHead(make_shared<Word> (grandchild->GetChildren()[0]->GetLabel()));
								found = true;
							}
						}
					}
				}
		*/

				if (headTree->GetHead().get() != nullptr){
					child->SetHead(headTree->GetHead());
				/*
				 * nsubj doesn't have a head because there is no pre-terminal head -> get it from [^nsubj] (last leafNT)
				 * this should be solved with recursing to previous hypothesis !! todo !!
				 	[sent [root [nsubj [det [DT the]] [^nsubj]] [VBZ has]]]
					[sent [root [nsubj [det [DT the]] [^nsubj]] [VBZ has]]]
					[sent [root [nsubj [det [DT the]] [^nsubj [nn [NNP Prime]] [NNP Minister]]] [VBZ has]]]
				*/
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
				tree->SetHead(make_shared<Word> (headTree->GetChildren()[0]->GetLabel()));
				found = true;
			}
		}
		else {
			// recursively find heads for internal tree (for non-terminal children of the current node that are not pre-terminals)
			// Example: [prep [IN of] [pobj [det [DT the]] [NN session]]] -> head of pobj
			if (!headTree->IsTerminal()) // should never be the case we recursively reached a terminal
				FindHeadRecursively(headTree, previous_trees, childId);
		}

	}
	return found;
}

vector<string> SelPrefFeature::ProcessChild(
		TreePointer child, size_t childId,
		TreePointer currentNode,
		const std::vector<TreePointer> &previous_trees) const{
	vector<string> depTuple;
	string rel = ToString(&(child->GetLabel()));
	if(!child->IsTerminal()
			&& m_allowedLabels.find(rel) != m_allowedLabels.end()){
		// {rel, head, dep}

		// I extract tuples top-down looking at the child label
		// In this case prep is a modifier of a dep node (this structure make no sense?) -> should I check the parent label as well?
		// Head: ^root  gathered  Children heads: punct  .  prep  of  dep  in  ^root  gathered
		// prep_of in tokyo
		// SCORE: -6.280 -12551.850
		// [^root [VBN] [^root [dep [IN] [prep]] [punct [. .]]]]
		// [^root [VBN] [dep [IN] [prep]] [punct [. .]]]
		// [^root [VBN gathered] [dep [IN in] [prep [IN of] [pobj [NNP Tokyo]]]] [punct [. .]]]

		// Will need to deal with ^prep leafNT and finding pobj in previous hypothesis
		// -> should be solved by unbinarizing the ^leafNT recursively
		if (rel == "prep"){
			if (child->GetHead().get() == nullptr){
				//cout << "No prep head!" << endl;
				return depTuple;
			}
			rel = rel + "_" + ToString(child->GetHead().get());
			if(child->IsLeafNT() && childId != -1){ // -1 for child taken from prev hypothesis via ^NT
				// Since prep is LeafNT we don't know what are it's children so we need to look in previous Trees
				child = previous_trees[childId];
			}

			child = FindChildByLabel(child, "pobj");
			// if the prep node had no pobj then I don't know which is the dependent; skip this relation tuple
			// same chain gathered->in->Tokyo but in the second case the relation is not scored because the label is "dep" not "pobj"
			// this produces the same translation but the syntactic parse is different
			// [sent [root [VBN] [prep [IN] [pobj]] [punct [. .]]]]
			// [sent [root [VBN gathered] [prep [IN in] [pobj [NNP Tokyo]]] [punct [. .]]]]
			// [^root [VBN] [prep [IN] [dep]] [punct [. .]]]
			// [^root [VBN gathered] [prep [IN in] [dep [NNP Tokyo]]] [punct [. .]]]
			if(!child){
				//cout << "NO pobj child" << endl;
				return depTuple;
			}
		}
		if (child->GetHead().get() == nullptr){
			//cout << "No head in children!" << endl;
			return depTuple;
		}
		string dep;
		dep = FilterArg(ToString(child->GetHead().get()), m_lemmaMap);

		if (currentNode->GetHead().get() == nullptr){
			//cout << "No parent head!" << endl;
			return depTuple;
		}
		string head = FilterArg(ToString(currentNode->GetHead().get()), m_lemmaMap);

		//cout << "new tuple: " << rel << " " << head << " " << dep <<endl;
		depTuple = {rel, head, dep};
	}
	return depTuple;
}

/*
 * Returns tuples of parent-head-word child-head-word relation, if the child relation belongs to: nsubj, nsubjpass, dobj, iobj, prep
 * Iterates through all the nodes in the current hypothesis.
 * For prep child relation have to look for the head word of the pobj child
 * perhaps should consider: prepc: prepositional clausal modifier => “He purchased it without paying a premium” prepc_without(purchased, paying)
*/
void SelPrefFeature::MakeTuples(
		TreePointer tree, const std::vector<TreePointer> &previous_trees,
		vector<vector<string>> &depRelTuples) const{
	size_t childId = -1;
	queue< TreePointer> BFSQueue;
	BFSQueue.push(tree);
	//cout << "Start tuples " << endl;
	while(!BFSQueue.empty()){
		bool prev = false;
		auto currentNode = BFSQueue.front();
		BFSQueue.pop();
		for (auto child : currentNode->GetChildren()){
			if(child->IsLeafNT()){
				childId ++;
				//todo !!!!!
				// if starts with ^ add all it's children from the prev hypothesis to the queue and signal the recurssion to stop at their level
				// Check Rico's emnlp2014 paper on how to solve this with unbinarization
	/*			if(child->GetLabel().GetString(0).as_string()[0] == '^'){
					for (auto grandchild :  previous_trees[childId]->GetChildren()){ // get them from previous_trees
						vector<string> tuple = ProcessChild(grandchild, -1, currentNode.first, previous_trees);
						if (tuple.size() != 0)
							depRelTuples.push_back(tuple);
						}
					continue;

						// maybe some case like (but children might be scored twice):
						// [root [VB gave] [^root]]
						//  [^root [dobj [det [DT the]] [NN book]] [iobj [dep [TO to]] [PRP you]]]

						// -> need to unbinarize ^leafNT before MakeTuples to get: prep_in gathered Tokyo (since I'm extracting top-down)
			  	  	  	// Head: nsubjpass  gathered  Children heads: nsubjpass  gathered
						// [nsubjpass [VBN] [^nsubjpass]]
						// [nsubjpass [VBN] [^nsubjpass]]
						// [nsubjpass [VBN gathered] [^nsubjpass [prep [IN in] [pobj [NNP Tokyo]]] [punct [. .]]]]


						// problem for: ^root is very long and you will score children twice
						// [^root [aux] [^root [punct [, ,]] [^root [prep] [^root]]] ]
						// [^root [aux] [punct [, ,]] [prep] [^root]]
						// [^root [aux [VBD met] [nsubj [PRP itself]]] [punct [, ,]] [prep [IN of] [pobj [NNP Tokyo]]] [^root [punct [, ,]] [dep [VBD reflected] [prep [IN in] [pobj [det [DT the]] [NNS waters] [prep [IN of] [pobj [det [DT a]] [amod [JJ romantic]] [NN canal]]]]]] [punct [, ,]] [parataxis [nsubj [DT this]] [cop [VBZ is]] [det [DT a]] [amod [JJ unique]] [NN opportunity] [vmod [aux [TO to]] [VB immerse] [dobj [PRP yourself]] [prep [IN in] [pobj [NNS layers] [prep [IN of] [pobj [NN silk] [punct [, ,]] [conj:and [NNS brocades]] [punct [, ,]] [conj:and [NNS damasks]] [cc [CC and]] [conj:and [NN gilt]]]]]]] [dep [VBZ mirrors] [ccomp [nsubj [IN that]] [VBP echo] [dobj [det [DT a]] [NNP Serenissima] [prep [IN of] [pobj [JJ long] [advmod [RB ago]]]] [punct [, ,]] [appos [det [DT a]] [NN city] [prep [IN of] [pobj [NN travel] [cc [CC and]] [conj:and [NN commerce]]]] [prep [IN with] [pobj [det [DT the]] [amod [JJ entire]] [NNP Orient]]]]]]]] [punct [. .]] [parataxis [nsubjpass [det [DT the]] [nn [NNP Palazzo]] [nn [NNP del]] [nn [NNP Doge]] [NNP Falier] [punct [, ,]] [vmod [VBD situated] [advmod [RB close] [prep [TO to] [pobj [nn [NNP Piazza]] [nn [NNP S.]] [NNP Marco] [cc [CC and]] [conj:and [det [DT the]] [nn [NNP Rialto]] [NNP Bridge]]]]]] [punct [, ,]]] [aux [VBZ has]] [auxpass [VBN been]] [VBN restored] [prep [IN with] [pobj [NN passion] [cc [CC and]] [conj:and [NN attention]]]] [prep [TO to] [pobj [NN detail]]]] [punct [. .]]]]
				}
		*/

			}
			if(!child->IsLeafNT()) //don't go extracting tuples from previous hypothesis
				BFSQueue.push(child);

			vector<string> tuple = ProcessChild(child, childId, currentNode, previous_trees);
			if (tuple.size() != 0)
				depRelTuples.push_back(tuple);
		}
	}
}

/*
 * Querying the Witten Bell dependency language model with KenLM
 */
// todo: create cache for already seen dep tuples -> should cache all 4 scores
// todo: marked nodes as scored

float SelPrefFeature::GetWBScore(vector<string>& depRel) const{
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
		for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
		  const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
		  if (word.IsNonTerminal()) {
			size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
			const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
			const SelPrefState* prev = static_cast<const SelPrefState*>(prevHypo->GetFFState(featureID));
			const TreePointer prev_tree = prev->GetTree();
			previous_trees.push_back(prev_tree);

		  }
		}

		if(m_unbinarize)
			UnbinarizeTree(mytree);

		// find head word for the root of the current hypothesis and the head words for the internal non-terminals
		size_t childId = 0;
		FindHeadRecursively(mytree, previous_trees, childId);

		//PrintHeadWords(mytree);

		// Extract dep rel tuples for the current syntax tree
		vector<vector<string>> depRelTuples;
		MakeTuples(mytree, previous_trees, depRelTuples);

		// Compute hash for current hypothesis
		//std::hash<vector<vector<string>>> hash_fn;
		size_t depRelHash = 0;
		boost::hash_combine(depRelHash, depRelTuples);
		//cout << "Hash value: " << depRelHash << endl;

		// Compute feature function scores
		float tuplesCounter = depRelTuples.size()*1.0;
		float scoreWB = 0.0;
		float scoreMI = 0.0;
		for (auto tuple: depRelTuples){
			scoreWB += GetWBScore(tuple);
			if (m_MIModelFile != ""){
				auto it_MI = m_MIModel->find(tuple);
				if(it_MI!=m_MIModel->end()){
					scoreMI += it_MI->second[0];
					// inverse score available as well
					//scoreMIInv += it_MI->second[1];
				}
			}
	/*		for (auto &elem: tuple)
				cout << elem << " ";
			cout <<endl;
			SelPrefFeature::score += score;
			cout << "SCORE: " << score << " "<< SelPrefFeature::score << endl;
	*/
		}

/*
		if(depRelTuples.size() !=0){
			std::cout << *tree << endl;
			std::cout <<  mytree->GetString() << endl;
		}
*/
		//std::cout << *tree << endl;
		//std::cout <<  mytree->GetString() << endl;


		mytree->Combine(previous_trees);

		//if(depRelTuples.size() !=0)
		//	std::cout << mytree->GetString() <<endl;
		//if(*tree == "[^root [nsubj [det [DT the]] [^nsubj]] [^root [VBZ has] [^root [dobj] [^root]]]]")
		//	exit(0);

		//if(*tree == "[sent [root [VB] [^root [dobj] [^root [prep [IN in]] [^root [prep] [punct [. .]]]]]]]")
		//	exit(0);

		vector<float> scores = {tuplesCounter};
		if(m_MIModelFile != "")
			scores.push_back(scoreMI);
		if(m_modelFileARPA != "")
			scores.push_back(scoreWB);
		accumulator->PlusEquals(this,scores);


		return new SelPrefState(mytree, depRelHash);
	}
	else {
	    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
	}

}


}
