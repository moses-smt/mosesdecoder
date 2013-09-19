///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This file is part of ModelBlocks. Copyright 2009, ModelBlocks developers. //
//                                                                           //
//    ModelBlocks is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by   //
//    the Free Software Foundation, either version 3 of the License, or      //
//    (at your option) any later version.                                    //
//                                                                           //
//    ModelBlocks is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//    GNU General Public License for more details.                           //
//                                                                           //
//    You should have received a copy of the GNU General Public License      //
//    along with ModelBlocks.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                           //
//    ModelBlocks developers designate this particular file as subject to    //
//    the "Moses" exception as provided by ModelBlocks developers in         //
//    the LICENSE file that accompanies this code.                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

template<class B, class T>
class Tree : public T {
 private:
  // Data members...
  SimpleHash<B,Tree<B,T>*> apt;
  static const Tree<B,T> tDummy;
 public:
  // Constructor / destructor methods...
  ~Tree ( )                  { for(typename SimpleHash<B,Tree<B,T>*>::iterator i=apt.begin(); i!=apt.end(); i++) delete i->second; }
  Tree  ( )                  { }
//  Tree  ( const Tree<T>& t ) { ptL = (t.ptL) ? new Tree<T>(*t.ptL) : NULL;
//                               ptR = (t.ptR) ? new Tree<T>(*t.ptR) : NULL; }
  // Extraction methods...
  const bool       isTerm    ( ) const { return (apt.empty()); }
  const Tree<B,T>& getBranch ( const B& b ) const { return (apt.find(b)!=apt.end()) ? *apt.find(b)->second : tDummy; }
  // Specification methods...
  Tree<B,T>& setBranch       ( const B& b )       { if (apt.find(b)==apt.end()) apt[b]=new Tree<B,T>(); return *apt[b]; }
};
template<class B, class T> const Tree<B,T> Tree<B,T>::tDummy;// = Tree<B,T>();

