#include <iostream>

#define BOOST_TEST_MODULE MertForestRescore
#include <boost/test/unit_test.hpp>

#include "Hypergraph.h"

using namespace std;
using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(prune)
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

  SparseVector weights;
  weights.set(f1,2);
  weights.set(f2,1);

  Graph pruned(vocab);
  graph.Prune(&pruned, weights, 5);

  BOOST_CHECK_EQUAL(5, pruned.EdgeSize());
  BOOST_CHECK_EQUAL(4, pruned.VertexSize());

  //edges retained should be best path (<s> ab jk </s>) and hi
  BOOST_CHECK_EQUAL(1, pruned.GetVertex(0).GetIncoming().size());
  BOOST_CHECK_EQUAL(2, pruned.GetVertex(1).GetIncoming().size());
  BOOST_CHECK_EQUAL(1, pruned.GetVertex(2).GetIncoming().size());
  BOOST_CHECK_EQUAL(1, pruned.GetVertex(3).GetIncoming().size());

  const Edge* edge;

  edge =  pruned.GetVertex(0).GetIncoming()[0];
  BOOST_CHECK_EQUAL(1, edge->Words().size());
  BOOST_CHECK_EQUAL(words[0], edge->Words()[0]);

  edge =  pruned.GetVertex(1).GetIncoming()[0];
  BOOST_CHECK_EQUAL(3, edge->Words().size());
  BOOST_CHECK_EQUAL((Vocab::Entry*)NULL, edge->Words()[0]);
  BOOST_CHECK_EQUAL(words[2]->first, edge->Words()[1]->first);
  BOOST_CHECK_EQUAL(words[3]->first, edge->Words()[2]->first);

  edge =  pruned.GetVertex(1).GetIncoming()[1];
  BOOST_CHECK_EQUAL(3, edge->Words().size());
  BOOST_CHECK_EQUAL((Vocab::Entry*)NULL, edge->Words()[0]);
  BOOST_CHECK_EQUAL(words[9]->first, edge->Words()[1]->first);
  BOOST_CHECK_EQUAL(words[10]->first, edge->Words()[2]->first);

  edge =  pruned.GetVertex(2).GetIncoming()[0];
  BOOST_CHECK_EQUAL(3, edge->Words().size());
  BOOST_CHECK_EQUAL((Vocab::Entry*)NULL, edge->Words()[0]);
  BOOST_CHECK_EQUAL(words[11]->first, edge->Words()[1]->first);
  BOOST_CHECK_EQUAL(words[12]->first, edge->Words()[2]->first);

  edge =  pruned.GetVertex(3).GetIncoming()[0];
  BOOST_CHECK_EQUAL(2, edge->Words().size());
  BOOST_CHECK_EQUAL((Vocab::Entry*)NULL, edge->Words()[0]);
  BOOST_CHECK_EQUAL(words[1]->first, edge->Words()[1]->first);





//  BOOST_CHECK_EQUAL(words[0], pruned.GetVertex(0).GetIncoming()[0].Words()[0]);


}
