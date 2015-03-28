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

#ifndef _NL_LIST_ //////////////////////////////////////////////////////////////
#define _NL_LIST_ //////////////////////////////////////////////////////////////

#include <cstdlib>

#define Listed(x) ListedObject<x>

////////////////////////////////////////////////////////////////////////////////
//
//  Container macros
//
////////////////////////////////////////////////////////////////////////////////

// Standard loop...
#define foreach(p,c) for ( p=(c).getNext(NULL); p!=NULL; p=(c).getNext(p) )

// True unless proven false...
#define setifall(y,p,c,x) for ( p=(c).getNext(NULL), y=true; p!=NULL && y; y &= (x), p=(c).getNext(p) )

// False unless proven true...
#define setifexists(y,p,c,x) for ( p=(c).getNext(NULL), y=false; p!=NULL && !y; y |= (x), p=(c).getNext(p) )


////////////////////////////////////////////////////////////////////////////////

template <class T>
class Ptr
  {
  private:
    T* ptObj ;
  public:
    Ptr ( )                                       { ptObj=NULL; }
    Ptr ( T* pt )                                 { ptObj=pt; }
    Ptr ( T& t  )                                 { ptObj=&t; }
    Ptr ( const Ptr<T>& pt )                      { ptObj=pt.ptObj; }
    bool    operator>  ( const Ptr<T>& pt ) const { return(ptObj>pt.ptObj); }
    bool    operator<  ( const Ptr<T>& pt ) const { return(ptObj<pt.ptObj); }
    bool    operator>= ( const Ptr<T>& pt ) const { return(ptObj>=pt.ptObj); }
    bool    operator<= ( const Ptr<T>& pt ) const { return(ptObj<=pt.ptObj); }
    bool    operator== ( const Ptr<T>& pt ) const { return(ptObj==pt.ptObj); }
    bool    operator!= ( const Ptr<T>& pt ) const { return(ptObj!=pt.ptObj); }
    Ptr<T>& operator=  ( const Ptr<T>& pt )       { ptObj=pt.ptObj; return *this; }
    T&      operator*  ( ) const                  { return *ptObj; }
    T*      operator-> ( ) const                  { return ptObj; }
  } ;


////////////////////////////////////////////////////////////////////////////////

template <class T>
class ListedObject ;

template <class T>
class List {
 private:

  ListedObject<T>* plotLast ;

 public:

  typedef ListedObject<T>*       iterator;
  typedef const ListedObject<T>* const_iterator;

  // Constructor and destructor methods...
  List  ( ) ;
  List  ( const T& ) ;
  List  ( const List<T>& ) ;
  List  ( const List<T>&, const List<T>& ) ;
  ~List ( ) ;

  // Overloaded operators...
  List<T>& operator=  ( const List<T>& ) ;
  List<T>& operator+= ( const List<T>& ) ;
  bool     operator== ( const List<T>& ) const ;
  bool     operator!= ( const List<T>& ) const ;

  // Specification methods...
  void     clear   ( ) ;
  T&       insert  ( Listed(T)* ) ;
  void     remove  ( Listed(T)* ) ;
  T&       add     ( ) ;
  T&       push    ( ) ;
  void     pop     ( ) ;
  Listed(T)* setFirst ( ) ;
  Listed(T)* setNext  ( Listed(T)* ) ;

  // Extraction methods...
  const_iterator begin ( ) const { return getNext(NULL); }
  const_iterator end   ( ) const { return NULL; }
  iterator& operator++ ( ) { *this=getNext(*this); return *this; }
  int        getCard  ( ) const ;
  Listed(T)* getFirst ( ) const ;
  Listed(T)* getSecond( ) const ;
  Listed(T)* getLast  ( ) const ;
  Listed(T)* getNext  ( const Listed(T)* ) const ;
  bool       contains ( const T& ) const ;
  bool       isEmpty  ( ) const ;

/*   // Input / output methods... */
/*   friend IStream operator>> ( pair<IStream,List<T>*> is_x, const char* psDlm ) { */
/*     IStream& is =  is_x.first; */
/*     List<T>& x  = *is_x.second; */
/*     if (IStream()!=is) */
/*       is = pair<IStream,T*>(is,&x.add())>>psDlm; */
/*     return is; */
/*   } */
} ;

////////////////////////////////////////////////////////////////////////////////

template <class T>
class ListedObject : public T
  {
  friend class List<T> ;

  private:

    ListedObject<T>* plotNext ;

  public:

    const ListedObject<T>* next ( ) const { return plotNext; }
    ListedObject ( ) { plotNext = NULL; }
    ListedObject ( const ListedObject<T>& lot )
      { T::operator=(lot); }
    ListedObject<T>& operator= ( const ListedObject<T>& lot )
      { T::operator=(lot); return(*this); }
    operator T() { return *this; }
  } ;


////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>::List ( )
  {
  plotLast = NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>::List ( const T& t )
  {
  plotLast = NULL ;

  add() = t ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>::List ( const List<T>& lt )
  {
  ListedObject<T>* pt ;

  plotLast = NULL ;

  foreach ( pt, lt )
    add() = *pt ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>::List ( const List<T>& lt1, const List<T>& lt2 )
  {
  ListedObject<T>* pt ;

  plotLast = NULL ;

  foreach ( pt, lt1 )
    add() = *pt ;
  foreach ( pt, lt2 )
    add() = *pt ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>::~List ( )
  {
    clear();
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
void List<T>::clear ( )
  {
  ListedObject<T>* plot ;
  ListedObject<T>* plot2 ;
  if ( NULL != (plot = plotLast) )
    do { plot2 = plot->plotNext ;
      ////fprintf(stderr,"list::destr %x\n",plot);
         delete plot ;
       } while ( plotLast != (plot = plot2) ) ;
  plotLast = NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>& List<T>::operator= ( const List<T>& lt )
  {
  Listed(T)* pt ;

  this->~List ( ) ;
  plotLast = NULL ;

  foreach ( pt, lt )
    add() = *pt ;

  return *this ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
List<T>& List<T>::operator+= ( const List<T>& lt )
  {
  Listed(T)* pt ;

  foreach ( pt, lt )
    add() = *pt ;

  return *this ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool List<T>::operator== ( const List<T>& lt ) const
  {
  Listed(T)* pt1 ;
  Listed(T)* pt2 ;

  for ( pt1 = getNext(NULL), pt2 = lt.getNext(NULL);
        pt1 != NULL && pt2 != NULL ;
        pt1 = getNext(pt1), pt2 = lt.getNext(pt2) )
    if ( !(*pt1 == *pt2) ) return false ;

  return ( pt1 == NULL && pt2 == NULL ) ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool List<T>::operator!= ( const List<T>& lt ) const
  {
  return !(*this == lt) ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
T& List<T>::insert ( Listed(T)* plotPrev )
  {
  ListedObject<T>* plot = new ListedObject<T> ;
  ////fprintf(stderr,"list::const %x\n",plot);

  if ( NULL != plotPrev )
    {
    plot->plotNext = plotPrev->plotNext ;
    plotPrev->plotNext = plot ;
    if ( plotLast == plotPrev )
      plotLast = plot ;
    }
  else if ( NULL != plotLast )
    {
    plot->plotNext = plotLast->plotNext ;
    plotLast->plotNext = plot ;
    }
  else
    {
    plot->plotNext = plot ;
    plotLast = plot ;
    }

  return *plot ;
  }

////////////////////////////////////////////////////////////////////////////////

/* DON'T KNOW WHY THIS DOESN'T WORK
template <class T>
void List<T>::remove ( Listed(T)* plot )
  {
  assert ( plot );
  assert ( plotLast );
  // If only one element...
  if ( plot->plotNext == plot )
    {
    assert ( plotLast == plot );
    plotLast = NULL;
fprintf(stderr,"list::delete1 %x\n",plot);
    delete plot;
    }
  // If more than one element...
  else
    {
    if ( plotLast == plot->plotNext ) plotLast = plot;
    Listed(T)* plotTemp = plot->plotNext;
    *plot = *(plot->plotNext);
fprintf(stderr,"list::delete2 %x\n",plotTemp);
    delete plotTemp;
    }
  }
*/

////////////////////////////////////////////////////////////////////////////////

template <class T>
T& List<T>::add ( )
  {
  ListedObject<T>* plot = new ListedObject<T> ;
  ////fprintf(stderr,"list::add %x\n",plot);

  if ( NULL != plotLast )
    {
    plot->plotNext = plotLast->plotNext ;
    plotLast->plotNext = plot ;
    plotLast = plot ;
    }
  else
    {
    plot->plotNext = plot ;
    plotLast = plot ;
    }

  return *plot ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
T& List<T>::push ( )
  {
  ListedObject<T>* plot = new ListedObject<T> ;
  ////fprintf(stderr,"list::push %x\n",plot);

  if ( NULL != plotLast )
    {
    plot->plotNext = plotLast->plotNext ;
    plotLast->plotNext = plot ;
    }
  else
    {
    plot->plotNext = plot ;
    plotLast = plot ;
    }

  return *plot ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
void List<T>::pop ( )
  {
  ListedObject<T>* plot = plotLast->plotNext ;

  if ( plot->plotNext == plot )
    plotLast = NULL ;
  else
    plotLast->plotNext = plot->plotNext ;

  ////fprintf(stderr,"list::pop %x\n",plot);
  delete plot ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
int List<T>::getCard ( ) const
  {
  Listed(T)* pt ;
  int        i = 0 ;

  foreach ( pt, *this )
    i++ ;

  return i ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::setFirst ( )
  {
  return ( NULL != plotLast ) ? plotLast->plotNext : NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::setNext ( ListedObject<T>* plot )
  {
  return ( NULL == plot && NULL != plotLast ) ? plotLast->plotNext :
         ( plot != plotLast ) ? plot->plotNext : NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::getFirst ( ) const
  {
  return ( NULL != plotLast ) ? plotLast->plotNext : NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::getSecond ( ) const
  {
  return getNext(getFirst()) ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::getLast ( ) const
  {
  return ( NULL != plotLast ) ? plotLast : NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
ListedObject<T>* List<T>::getNext ( const ListedObject<T>* plot ) const
  {
  return ( NULL == plot && NULL != plotLast ) ? plotLast->plotNext :
         ( plot != plotLast ) ? plot->plotNext : NULL ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool List<T>::contains ( const T& t ) const
  {
  ListedObject<T>* pt ;

  foreach ( pt, *this )
    if ( t == *pt ) return true ;

  return false ;
  }

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool List<T>::isEmpty ( ) const
  {
  return ( NULL == plotLast ) ;
  }

#endif //_NL_LIST_ /////////////////////////////////////////////////////////////

