// vim:tabstop=2

/***********************************************************************
  Moses - factored hierarchical phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include "relax-parse.h"
#include "tables-core.h"

using namespace std;
using namespace MosesTraining;

int main(int argc, char* argv[])
{
  init( argc, argv ); // initialize from switches, set flags

  // loop through all sentences
  int i=0;
  string inBufferString;
  while(cin.peek() != EOF) {
    getline(cin,inBufferString);
    i++;
    if (i%1000 == 0) cerr << "." << flush;
    if (i%10000 == 0) cerr << ":" << flush;
    if (i%100000 == 0) cerr << "!" << flush;

    // process into syntax tree representation
    set< string > labelCollection;         // set of labels, not used
    map< string, int > topLabelCollection; // count of top labels, not used
    SyntaxTree tree;
    ProcessAndStripXMLTags( inBufferString, tree, labelCollection, topLabelCollection, false );
    vector< string > inWords = tokenize( inBufferString.c_str() );

    // output tree
    // cerr << "BEFORE:" << endl << tree;

    ParentNodes parents = tree.Parse();

    // execute selected grammar relaxation schemes
    if (leftBinarizeFlag)
      LeftBinarize( tree, parents );

    if (rightBinarizeFlag)
      RightBinarize( tree, parents );

    if (SAMTLevel>0)
      SAMT( tree, parents );

    // output tree
    // cerr << "AFTER:" << endl << tree;

    store( tree, inWords );
  }
}

// initialize settings from switches

void init(int argc, char* argv[])
{
  cerr << "Parse Relaxer v1.0, written by Philipp Koehn\n";
  cerr << "adds additional constituents to a parse tree\n";

  if (argc < 2) {
    cerr << "syntax: relax-parse < in-parse > out-parse ["
         << " --LeftBinarize | --RightBinarize |"
         << " --SAMT 1-4 ]" << endl;
    exit(1);
  }

  for(int i=1; i<argc; i++) {
    // add constituents with binarization
    if (strcmp(argv[i],"--LeftBinarize") == 0) {
      leftBinarizeFlag = true;
    } else if (strcmp(argv[i],"--RightBinarize") == 0) {
      rightBinarizeFlag = true;
    }

    // add constituents according to samt (Zollmann/Venugopal)
    else if (strcmp(argv[i],"--SAMT") == 0) {
      SAMTLevel = atoi( argv[++i] );
      cerr << "using SAMT grammar, level " <<  SAMTLevel << endl;
    }

    // error
    else {
      cerr << "relax-grammar: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }
}

void store( SyntaxTree &tree, vector< string > &words )
{
  // output words
  for( size_t i=0; i<words.size(); i++ ) {
    if (i>0) {
      cout << " ";
    }
    cout << words[i];
  }

  // output tree nodes
  vector< SyntaxNode* > nodes = tree.GetAllNodes();
  for( size_t i=0; i<nodes.size(); i++ ) {
    cout << " <tree span=\"" << nodes[i]->GetStart()
         << "-" << nodes[i]->GetEnd()
         << "\" label=\"" << nodes[i]->GetLabel()
         << "\"/>";
  }
  cout << endl;
}

void LeftBinarize( SyntaxTree &tree, ParentNodes &parents )
{
  for(ParentNodes::const_iterator p = parents.begin(); p != parents.end(); p++) {
    const SplitPoints &point = *p;
    if (point.size() > 3) {
      const vector< SyntaxNode* >& topNodes
      = tree.GetNodes( point[0], point[point.size()-1]-1);
      string topLabel = topNodes[0]->GetLabel();

      for(size_t i=2; i<point.size()-1; i++) {
        // cerr << "LeftBin  " << point[0] << "-" << (point[point.size()-1]-1) << ": " << point[0] << "-" << point[i]-1 << " ^" << topLabel << endl;
        tree.AddNode( point[0], point[i]-1, "^" + topLabel );
      }
    }
  }
}

void RightBinarize( SyntaxTree &tree, ParentNodes &parents )
{
  for(ParentNodes::const_iterator p = parents.begin(); p != parents.end(); p++) {
    const SplitPoints &point = *p;
    if (point.size() > 3) {
      int endPoint = point[point.size()-1]-1;
      const vector< SyntaxNode* >& topNodes
      = tree.GetNodes( point[0], endPoint);
      string topLabel = topNodes[0]->GetLabel();

      for(size_t i=1; i<point.size()-2; i++) {
        // cerr << "RightBin " << point[0] << "-" << (point[point.size()-1]-1) << ": " << point[i] << "-" << endPoint << " ^" << topLabel << endl;
        tree.AddNode( point[i], endPoint, "^" + topLabel );
      }
    }
  }
}

void SAMT( SyntaxTree &tree, ParentNodes &parents )
{
  int numWords = tree.GetNumWords();

  SyntaxTree newTree; // to store new nodes

  // look through parents to combine children
  for(ParentNodes::const_iterator p = parents.begin(); p != parents.end(); p++) {
    const SplitPoints &point = *p;

    // neighboring childen: DET+ADJ
    if (point.size() >= 3) {
      // cerr << "complex parent: ";
      // for(int i=0;i<point.size();i++) cerr << point[i] << " ";
      // cerr << endl;

      for(size_t i = 0; i+2 < point.size(); i++) {
        // cerr << "\tadding " << point[i] << ";" << point[i+1] << ";" << (point[i+2]-1) << ": " << tree.GetNodes(point[i  ],point[i+1]-1)[0]->GetLabel() << "+" << tree.GetNodes(point[i+1],point[i+2]-1)[0]->GetLabel() << endl;

        newTree.AddNode( point[i],point[i+2]-1,
                         tree.GetNodes(point[i  ],point[i+1]-1)[0]->GetLabel()
                         + "+" +
                         tree.GetNodes(point[i+1],point[i+2]-1)[0]->GetLabel() );
      }
    }
    if (point.size() >= 4) {
      int ps = point.size();
      string topLabel = tree.GetNodes(point[0],point[ps-1]-1)[0]->GetLabel();

      // cerr << "\tadding " << topLabel + "\\" + tree.GetNodes(point[0],point[1]-1)[0]->GetLabel() << endl;
      newTree.AddNode( point[1],point[ps-1]-1,
                       topLabel
                       + "\\" +
                       tree.GetNodes(point[0],point[1]-1)[0]->GetLabel() );

      // cerr << "\tadding " << topLabel + "/" + tree.GetNodes(point[ps-2],point[ps-1]-1)[0]->GetLabel() << endl;
      newTree.AddNode( point[0],point[ps-2]-1,
                       topLabel
                       + "/" +
                       tree.GetNodes(point[ps-2],point[ps-1]-1)[0]->GetLabel() );
    }
  }

  // rules for any bordering constituents...
  for(int size = 2; size < numWords; size++) {
    for(int start = 0; start < numWords-size+1; start++) {
      int end = start+size-1;
      bool done = false;

      if (tree.HasNode( start,end ) || newTree.HasNode( start,end )
          || SAMTLevel <= 1) {
        continue;
      }

      // if matching two adjacent parse constituents: use ++

      for(int mid=start+1; mid<=end && !done; mid++) {
        if (tree.HasNode(start,mid-1) && tree.HasNode(mid,end)) {
          // cerr << "\tadding " << tree.GetNodes(start,mid-1)[0]->GetLabel() << "++" << tree.GetNodes(mid,  end  )[0]->GetLabel() << endl;

          newTree.AddNode( start, end,
                           tree.GetNodes(start,mid-1)[0]->GetLabel()
                           + "++" +
                           tree.GetNodes(mid,  end  )[0]->GetLabel() );
          done = true;
        }
      }
      if (done) continue;

      // if matching a constituent A right-minus const. B: use A//B
      for(int postEnd=end+1; postEnd<numWords && !done; postEnd++) {
        if (tree.HasNode(start,postEnd) && tree.HasNode(end+1,postEnd)) {
          newTree.AddNode( start, end,
                           tree.GetNodes(start,postEnd)[0]->GetLabel()
                           + "//" +
                           tree.GetNodes(end+1,postEnd)[0]->GetLabel() );
          done = true;
        }
      }
      if (done) continue;

      // if matching a constituent A left-minus constituent B: use A\\B
      for(int preStart=start-1; preStart>=0; preStart--) {
        if (tree.HasNode(preStart,end) && tree.HasNode(preStart,start-1)) {
          // cerr << "\tadding " << tree.GetNodes(preStart,end    )[0]->GetLabel() << "\\\\" <<tree.GetNodes(preStart,start-1)[0]->GetLabel() << endl;
          newTree.AddNode( start, end,
                           tree.GetNodes(preStart,end    )[0]->GetLabel()
                           + "\\\\" +
                           tree.GetNodes(preStart,start-1)[0]->GetLabel() );
          done = true;
        }
      }
      if (done) continue;

      // if matching three consecutive constituents, use double-plus
      // SAMT Level 3, not yet implemented

      // else: assign default category _FAIL
      if (SAMTLevel>=4) {
        newTree.AddNode( start, end, "_FAIL" );
      }
    }
  }

  // adding all new nodes
  vector< SyntaxNode* > nodes = newTree.GetAllNodes();
  for( size_t i=0; i<nodes.size(); i++ ) {
    tree.AddNode( nodes[i]->GetStart(), nodes[i]->GetEnd(), nodes[i]->GetLabel());
  }
}
