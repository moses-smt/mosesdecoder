#include <iostream>

#include "util/tokenize_piece.hh"

#include "ForestRescore.h"
#include "MiraFeatureVector.h"

#define BOOST_TEST_MODULE MertForestRescore
#include <boost/test/unit_test.hpp>



using namespace std;
using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(viterbi_simple_lattice)
{
  Vocab vocab;
  WordVec words;
  string wordStrings[] =
  {"<s>", "</s>", "a", "b", "c", "d", "e", "f", "g"};
  for (size_t i = 0; i < 9; ++i) {
    words.push_back(&(vocab.FindOrAdd((wordStrings[i]))));
  }

  const string f1 = "foo";
  const string f2 = "bar";
  Graph graph(vocab);
  graph.SetCounts(5,5);

  Edge* e0 = graph.NewEdge();
  e0->AddWord(words[0]);
  e0->AddFeature(f1, 2.0);

  Vertex* v0 = graph.NewVertex();
  v0->AddEdge(e0);

  Edge* e1 = graph.NewEdge();
  e1->AddWord(NULL);
  e1->AddChild(0);
  e1->AddWord(words[2]);
  e1->AddWord(words[3]);
  e1->AddFeature(f1, 1.0);
  e1->AddFeature(f2, 3.0);

  Vertex* v1 = graph.NewVertex();
  v1->AddEdge(e1);

  Edge* e2 = graph.NewEdge();
  e2->AddWord(NULL);
  e2->AddChild(1);
  e2->AddWord(words[4]);
  e2->AddWord(words[5]);
  e2->AddFeature(f2, 2.5);

  Vertex* v2 = graph.NewVertex();
  v2->AddEdge(e2);

  Edge* e3 = graph.NewEdge();
  e3->AddWord(NULL);
  e3->AddChild(2);
  e3->AddWord(words[6]);
  e3->AddWord(words[7]);
  e3->AddWord(words[8]);
  e3->AddFeature(f1, -1);

  Vertex* v3 = graph.NewVertex();
  v3->AddEdge(e3);

  Edge* e4 = graph.NewEdge();
  e4->AddWord(NULL);
  e4->AddChild(3);
  e4->AddWord(words[1]);
  e3->AddFeature(f2, 0.5);

  Vertex* v4 = graph.NewVertex();
  v4->AddEdge(e4);

  ReferenceSet references;
  references.AddLine(0, "a b c k e f o", vocab);
  HgHypothesis modelHypo;
  vector<FeatureStatsType> bg(kBleuNgramOrder*2+1);
  SparseVector weights;
  weights.set(f1,2);
  weights.set(f2,1);
  Viterbi(graph, weights, 0, references, 0, bg, &modelHypo);
  BOOST_CHECK_CLOSE(2.0,modelHypo.featureVector.get(f1), 0.0001);
  BOOST_CHECK_CLOSE(6.0,modelHypo.featureVector.get(f2), 0.0001);

  BOOST_CHECK_EQUAL(words[0]->first, modelHypo.text[0]->first);
  BOOST_CHECK_EQUAL(words[2]->first, modelHypo.text[1]->first);
  BOOST_CHECK_EQUAL(words[3]->first, modelHypo.text[2]->first);
  BOOST_CHECK_EQUAL(words[4]->first, modelHypo.text[3]->first);
  BOOST_CHECK_EQUAL(words[5]->first, modelHypo.text[4]->first);
  BOOST_CHECK_EQUAL(words[6]->first, modelHypo.text[5]->first);
  BOOST_CHECK_EQUAL(words[7]->first, modelHypo.text[6]->first);
  BOOST_CHECK_EQUAL(words[8]->first, modelHypo.text[7]->first);
  BOOST_CHECK_EQUAL(words[1]->first, modelHypo.text[8]->first);
}



BOOST_AUTO_TEST_CASE(viterbi_3branch_lattice)
{
  Vocab vocab;
  WordVec words;
  string wordStrings[] =
  {"<s>", "</s>", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k"};
  for (size_t i = 0; i < 13; ++i) {
    words.push_back(&(vocab.FindOrAdd((wordStrings[i]))));
  }

  const string f1 = "foo";
  const string f2 = "bar";
  Graph graph(vocab);
  graph.SetCounts(5,8);

  Edge* e0 = graph.NewEdge();
  e0->AddWord(words[0]);

  Vertex* v0 = graph.NewVertex();
  v0->AddEdge(e0);

  Edge* e1 = graph.NewEdge();
  e1->AddWord(NULL);
  e1->AddChild(0);
  e1->AddWord(words[2]);
  e1->AddWord(words[3]);
  e1->AddFeature(f1,1);
  e1->AddFeature(f2,1);
  Edge* e5 = graph.NewEdge();
  e5->AddWord(NULL);
  e5->AddChild(0);
  e5->AddWord(words[9]);
  e5->AddWord(words[10]);
  e5->AddFeature(f1,2);
  e5->AddFeature(f2,-2);

  Vertex* v1 = graph.NewVertex();
  v1->AddEdge(e1);
  v1->AddEdge(e5);
  v1->SetSourceCovered(1);

  Edge* e2 = graph.NewEdge();
  e2->AddWord(NULL);
  e2->AddChild(1);
  e2->AddWord(words[4]);
  e2->AddWord(words[5]);
  e2->AddFeature(f2,3);

  Vertex* v2 = graph.NewVertex();
  v2->AddEdge(e2);
  v2->SetSourceCovered(3);

  Edge* e3 = graph.NewEdge();
  e3->AddWord(NULL);
  e3->AddChild(2);
  e3->AddWord(words[6]);
  e3->AddWord(words[7]);
  e3->AddWord(words[8]);
  e3->AddFeature(f1,1);
  Edge* e6 = graph.NewEdge();
  e6->AddWord(NULL);
  e6->AddChild(2);
  e6->AddWord(words[9]);
  e6->AddWord(words[12]);
  e6->AddFeature(f2,1);
  Edge* e7 = graph.NewEdge();
  e7->AddWord(NULL);
  e7->AddChild(1);
  e7->AddWord(words[11]);
  e7->AddWord(words[12]);
  e7->AddFeature(f1,2);
  e7->AddFeature(f2,3);

  Vertex* v3 = graph.NewVertex();
  v3->AddEdge(e3);
  v3->AddEdge(e6);
  v3->AddEdge(e7);
  v3->SetSourceCovered(5);

  Edge* e4 = graph.NewEdge();
  e4->AddWord(NULL);
  e4->AddChild(3);
  e4->AddWord(words[1]);

  Vertex* v4 = graph.NewVertex();
  v4->AddEdge(e4);
  v4->SetSourceCovered(6);

  /*Paths     || foo || bar || s(2,1)
   ab cd hk   || 1   ||  5  || 7
   hi cd hk   || 2   ||  2  || 6
   ab jk      || 3   ||  4  || 10
   hi jk      || 4   ||  1  || 9
   ab cd efg  || 2   ||  4  || 8
   hi cd efg  || 3   ||  1  || 7
  */

  ReferenceSet references;
  references.AddLine(0, "a b c d h k", vocab);
  HgHypothesis modelHypo;
  vector<FeatureStatsType> bg(kBleuNgramOrder*2+1, 0.1);
  SparseVector weights;
  weights.set(f1,2);
  weights.set(f2,1);
  Viterbi(graph, weights, 0, references, 0, bg, &modelHypo);
  BOOST_CHECK_CLOSE(3.0,modelHypo.featureVector.get(f1), 0.0001);
  BOOST_CHECK_CLOSE(4.0,modelHypo.featureVector.get(f2), 0.0001);

  BOOST_CHECK_EQUAL(6, modelHypo.text.size());

  //expect ab jk
  BOOST_CHECK_EQUAL(words[0]->first, modelHypo.text[0]->first);
  BOOST_CHECK_EQUAL(words[2]->first, modelHypo.text[1]->first);
  BOOST_CHECK_EQUAL(words[3]->first, modelHypo.text[2]->first);
  BOOST_CHECK_EQUAL(words[11]->first, modelHypo.text[3]->first);
  BOOST_CHECK_EQUAL(words[12]->first, modelHypo.text[4]->first);
  BOOST_CHECK_EQUAL(words[1]->first, modelHypo.text[5]->first);


  HgHypothesis hopeHypo;
  Viterbi(graph, weights, 1, references, 0, bg, &hopeHypo);
  //expect abcdhk
  BOOST_CHECK_EQUAL(8, hopeHypo.text.size());

  BOOST_CHECK_EQUAL(words[0]->first, hopeHypo.text[0]->first);
  BOOST_CHECK_EQUAL(words[2]->first, hopeHypo.text[1]->first);
  BOOST_CHECK_EQUAL(words[3]->first, hopeHypo.text[2]->first);
  BOOST_CHECK_EQUAL(words[4]->first, hopeHypo.text[3]->first);
  BOOST_CHECK_EQUAL(words[5]->first, hopeHypo.text[4]->first);
  BOOST_CHECK_EQUAL(words[9]->first, hopeHypo.text[5]->first);
  BOOST_CHECK_EQUAL(words[12]->first, hopeHypo.text[6]->first);
  BOOST_CHECK_EQUAL(words[1]->first, hopeHypo.text[7]->first);

  BOOST_CHECK_EQUAL(kBleuNgramOrder*2+1, hopeHypo.bleuStats.size());
  BOOST_CHECK_EQUAL(6, hopeHypo.bleuStats[0]);
  BOOST_CHECK_EQUAL(6, hopeHypo.bleuStats[1]);
  BOOST_CHECK_EQUAL(5, hopeHypo.bleuStats[2]);
  BOOST_CHECK_EQUAL(5, hopeHypo.bleuStats[3]);
  BOOST_CHECK_EQUAL(4, hopeHypo.bleuStats[4]);
  BOOST_CHECK_EQUAL(4, hopeHypo.bleuStats[5]);
  BOOST_CHECK_EQUAL(3, hopeHypo.bleuStats[6]);
  BOOST_CHECK_EQUAL(3, hopeHypo.bleuStats[7]);
  BOOST_CHECK_EQUAL(6, hopeHypo.bleuStats[8]);
}

