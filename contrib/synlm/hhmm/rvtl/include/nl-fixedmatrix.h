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


//basically a SafeArray2D with operators defined
template<class T>
class Matrix : public SafeArray2D<Id<int>,Id<int>,T> {
  //public:
  //int xSize;
  //int ySize;
 public:
  // Constructor / destructor methods...
  //~Matrix( )                           { delete[] at; }
  Matrix ( )                         : SafeArray2D<Id<int>,Id<int>,T>( )     { }//{ xSize=0; ySize=0;  }
  Matrix (int x, int y)              : SafeArray2D<Id<int>,Id<int>,T>(x,y)   { }//{ xSize=x; ySize=y; }
  Matrix (int x, int y, const T& t)  : SafeArray2D<Id<int>,Id<int>,T>(x,y,t) { }//{ xSize=x; ySize=y; }
  Matrix (const Matrix& a)           : SafeArray2D<Id<int>,Id<int>,T>(a.xSize(),a.ySize()) { //xSize=a.xSize; ySize=a.ySize;
                                                                               for(int i=0;i<xSize();i++) for(int j=0;j<ySize();j++) this->set(i,j)=a.get(i,j); }
  // Specification methods...
  //Matrix& operator= ( const Matrix<T>& sat )
  //  { xSize=sat.xSize; ySize=sat.ySize; //at=new T[xSize*ySize];
  //    for(int i=0;i<xSize;i++) for(int j=0;j<ySize;j++) set(i,j)=sat.at[i]; return *this; }
  void  init ( int x,int y )                   { (*this)=Matrix<T>(x,y,T()); }//xSize=x; ySize=y; }
  void  init ( int x,int y,const T& t )        { (*this)=Matrix<T>(x,y,t); }//xSize=x; ySize=y; }
  void  reset()                                { (*this)=Matrix<T>( ); }//xSize=0; ySize=0; }

  // Inherited methods
  //T&    set  ( const X1& x,const X2& y);
  //const T& get (const X1& x,const X2& y) const;

  int xSize( ) const { return this->getxSize(); }
  int ySize( ) const { return this->getySize(); }

  // Math...
  friend Matrix<T> operator* ( const Matrix<T>& a, const Matrix<T>& b ) {
    if (a.ySize()!=b.xSize()) {
      cerr<<"ERROR: matrix multiplication requires matching inner indices; "<<a.xSize()<<"x"<<a.ySize()<<" "<<b.xSize()<<"x"<<b.ySize()<<endl;
      #ifndef NDEBUG
      cerr<<" a= "<<a<<"\n\n b= "<<b<<endl;
      #endif
      return Matrix<T>();
    }
    Matrix mOut(a.xSize(),b.ySize(),T());
    for (int i=0; i<a.xSize(); i++ ){
      for (int k=0; k<a.ySize(); k++ ) {
	for (int j=0; j<b.ySize(); j++ ) {
	  mOut.set(i,j) += a.get(Id<int>(i),Id<int>(k))*b.get(Id<int>(k),Id<int>(j));
	}
      }
    }
    //cerr<<" a= "<<a<<"\n b= "<<b<<"\n c= "<<mOut<<endl<<endl;
    return mOut;
  }
  friend Matrix<T> operator& ( const Matrix<T>& a, const Matrix<T>& b ) {
    if (a.xSize()!=b.xSize() || a.ySize()!=b.ySize()) {
      cerr<<"ERROR: pt-by-pt multiplication requires matching indices; "<<a.xSize()<<"x"<<a.ySize()<<" "<<b.xSize()<<"x"<<b.ySize()<<endl;
      #ifndef NDEBUG
      cerr<<" a= "<<a<<"\n\n b= "<<b<<endl;
      #endif
      return Matrix<T>();
    }
    Matrix mOut(a.xSize(),a.ySize(),T());
    for (int i=0; i<a.xSize(); i++ ){
	for (int j=0; j<b.ySize(); j++ ) {
	  mOut.set(i,j) += a.get(Id<int>(i),Id<int>(j))*b.get(Id<int>(i),Id<int>(j));
	}
    }
    //cerr<<" a= "<<a<<"\n b= "<<b<<"\n c= "<<mOut<<endl<<endl;
    return mOut;
  }
  friend Matrix<T> operator+ ( const Matrix<T>& a, const Matrix<T>& b ) {
    if (a.xSize()!=b.xSize() || a.ySize()!=b.ySize()) {
      cerr<<"ERROR: matrix addition requires matching dimensions"<<endl;
      return Matrix<T>();
    }
    Matrix mOut(a.xSize(),b.ySize(),T());
    for (int i=0; i<a.xSize(); i++ ){
      for (int j=0; j<a.ySize(); j++ ) {
	mOut.set(i,j) = a.get(Id<int>(i),Id<int>(j))+b.get(Id<int>(i),Id<int>(j));
      }
    }
    return mOut;
  }
  friend Matrix<T> operator- ( const Matrix<T>& a, const Matrix<T>& b ) {
    if (a.xSize()!=b.xSize() || a.ySize()!=b.ySize()) {
      cerr<<"ERROR: matrix subtraction requires matching dimensions"<<endl;
      //cerr<<"aSize="<<a.xSize<<","<<a.ySize()<<"     bSize="<<b.xSize<<","<<b.ySize()<<endl;
      //cerr<<" a= "<<a<<"\n b= "<<b<<"\n c= "<<mOut<<endl<<endl;
      return Matrix<T>();
    }
    Matrix mOut(a.xSize(),b.ySize(),T());
    for (int i=0; i<a.xSize(); i++ ){
      for (int j=0; j<a.ySize(); j++ ) {
	mOut.set(i,j) = a.get(Id<int>(i),Id<int>(j))-b.get(Id<int>(i),Id<int>(j));
      }
    }
    return mOut;
  }
  friend Matrix<T> operator* ( const Matrix<T>& a, const T& t ) {
    Matrix mOut(a.xSize(),a.ySize());
    for (int i=0; i<a.xSize(); i++ ){
      for (int j=0; j<a.ySize(); j++ ) {
	mOut.set(i,j) = a.get(Id<int>(i),Id<int>(j))*t;
      }
    }
    return mOut;
  }
  friend Matrix<T> operator+ ( const Matrix<T>& a, const T& t ) {
    Matrix mOut(a.xSize(),a.ySize());
    for (int i=0; i<a.xSize(); i++ ){
      for (int j=0; j<a.ySize(); j++ ) {
	mOut.set(i,j) = a.get(Id<int>(i),Id<int>(j))+t;
      }
    }
    return mOut;
  }
  friend Matrix<T> operator- ( const Matrix<T>& a, const T& t ) {
    Matrix mOut(a.xSize(),a.ySize());
    for (int i=0; i<a.xSize(); i++ ){
      for (int j=0; j<a.ySize(); j++ ) {
	mOut.set(i,j) = a.get(Id<int>(i),Id<int>(j))-t;
      }
    }
    return mOut;
  }

  // Scalar inf-norm (max) of matrix / vector...
  T infnorm ( ) const {
    T tOut = T();
    for (int i=0; i<xSize(); i++ ){
      for (int j=0; j<ySize(); j++ ) {
	if ( this->get(Id<int>(i),Id<int>(j))>tOut ) tOut = this->get(Id<int>(i),Id<int>(j));
      }
    }
    return tOut;
  }

  /*
  // Argmax of matrix / vector...  //NOT WORKING
  pair<int,int> argmax ( ) const {
    T tOut = T();
    pair<int,int> ij();
    for (int i=0; i<xSize(); i++ ){
      for (int j=0; j<ySize(); j++ ) {
	if ( this->get(Id<int>(i),Id<int>(j))>tOut ) {
	  tOut = this->get(Id<int>(i),Id<int>(j));
	  ij = make_pair(i,j);
	}
      }
    }
    return ij; //pair<int,int>( ij.getIndex(), ij.getIndex() );
  }
  */
  // Diagonal matrix of vector...
  friend Matrix<T> diag ( const Matrix<T>& a ) {
    Matrix mOut(a.xSize(),a.xSize(),T()); // output is n x n
    for (int i=0;i<a.xSize();i++) {
      for (int j=0;j<a.ySize();j++) {
        assert(j==0); // must be vector, n x 1
        mOut.set(Id<int>(i),Id<int>(i)) += a.get(Id<int>(i),Id<int>(j));
      }
    }
    return mOut;
  }

  // Ordering method (treat as bit string)...
  bool operator< ( const Matrix<T>& mt ) const {
    if (xSize()<mt.xSize() || ySize()<mt.ySize()) return true;
    if (xSize()>mt.xSize() || ySize()>mt.ySize()) return false;
    for (int i=0; i<xSize(); i++ ) {
      for (int j=0; j<ySize(); j++ ) {
	if ( this->get(Id<int>(i),Id<int>(j)) < mt.get(Id<int>(i),Id<int>(j)) ) return true;
	else if ( this->get(Id<int>(i),Id<int>(j)) > mt.get(Id<int>(i),Id<int>(j)) ) return false;
      }
    }
    return false;
  }
  bool operator== ( const Matrix<T>& a ) const {
    if (xSize()!=a.xSize() || ySize()!=a.ySize()) return false;
    for (int i=0;i<a.xSize();i++)
      for (int j=0;j<a.ySize();j++)
	if (this->get(Id<int>(i),Id<int>(j))!=a.get(Id<int>(i),Id<int>(j))) return false;
    return true;
  }

  // Input/output methods...
  friend ostream& operator<< ( ostream& os, const Matrix<T>& a ) {
    os<<"\n    ";
    for (int i=0;i<a.xSize();i++) {
      for (int j=0;j<a.ySize();j++) {
	os<<((j==0)?"":",")<<a.get(Id<int>(i),Id<int>(j));
      }
      os<<(i==a.xSize()-1?"\n":"\n    ");
    }
    return os;
  }
  friend String& operator<< ( String& str, const Matrix<T>& a ) {
    str<<"\n    ";
    for (int i=0;i<a.xSize();i++) {
      for (int j=0;j<a.ySize();j++) {
	str<<((j==0)?"":",")<<a.get(Id<int>(i),Id<int>(j));
      }
      str<<";";
    }
    return str;
  }
  string getString( ) const;

};
template <class T>
string Matrix<T>::getString() const {
  string str;
  for (int i=0;i<xSize();i++) {
    for (int j=0;j<ySize();j++) {
      str += ((j==0)?"":",");
      str += this->get(Id<int>(i),Id<int>(j));
    }
    str += ";";
  }
  return str;
}

