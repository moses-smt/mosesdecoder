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

template<class I,class J,class P>
class SparseMatrix : public SimpleHash<I,SimpleHash<J,P> > {
 public:

  typedef SimpleHash<I,SimpleHash<J,P> > Parent;

  //// Matrix / vector operator methods...
  friend SparseMatrix<I,J,P> operator* ( const SparseMatrix<I,J,P>& a, const SparseMatrix<I,J,P>& b ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator kit=a.get(i).begin(); kit!=a.get(i).end(); kit++ ) {
	I k = kit->first;
        for ( typename SimpleHash<J,P>::const_iterator jit=b.get(k).begin(); jit!=b.get(k).end(); jit++ ) {
	  I j = jit->first;
          if ( a.get(i).get(k)!=0 && b.get(k).get(j)!=0 )
            mOut.set(i).set(j) += a.get(i).get(k) * b.get(k).get(j);
        }
      }
    }
    return mOut;
  }
  friend SparseMatrix<I,J,P> operator+ ( const SparseMatrix<I,J,P>& a, const SparseMatrix<I,J,P>& b ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=a.get(i).begin(); jit!=a.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) = a.get(i).get(j);
      }
    }
    for ( typename Parent::const_iterator iit=b.begin(); iit!=b.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=b.get(i).begin(); jit!=b.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) += b.get(i).get(j);
      }
    }
    return mOut;
  }
  friend SparseMatrix<I,J,P> operator- ( const SparseMatrix<I,J,P>& a, const SparseMatrix<I,J,P>& b ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=a.get(i).begin(); jit!=a.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) = a.get(i).get(j);
      }
    }
    for ( typename Parent::const_iterator iit=b.begin(); iit!=b.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=b.get(i).begin(); jit!=b.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) -= b.get(i).get(j);
      }
    }
    return mOut;
  }
  // Matrix + scalar operators...
  friend SparseMatrix<I,J,P> operator+ ( const SparseMatrix<I,J,P>& a, const P& p ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=a.get(i).begin(); jit!=a.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) = a.get(i).get(j) + p;
      }
    }
    return mOut;
  }
  friend SparseMatrix<I,J,P> operator- ( const SparseMatrix<I,J,P>& a, const P& p ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=a.get(i).begin(); jit!=a.get(i).end(); jit++ ) {
	I j = jit->first;
        mOut.set(i).set(j) = a.get(i).get(j) - p;
      }
    }
    return mOut;
  }
  // Diagonal matrix of vector...
  friend SparseMatrix<I,J,P> diag ( const SparseMatrix<I,J,P>& a ) {
    SparseMatrix mOut;
    for ( typename Parent::const_iterator iit=a.begin(); iit!=a.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=a.get(i).begin(); jit!=a.get(i).end(); jit++ ) {
        I j = jit->first;
        assert(j==0); // must be vector
        mOut.set(i).set(i) += a.get(i).get(j);
      }
    }
    return mOut;
  }
  // Scalar inf-norm (max) of matrix / vector...
  P infnorm ( ) const {
    P pOut = 0;  // sparse matrix assumes some values are zero, so this is default infnorm.
    for ( typename Parent::const_iterator iit=Parent::begin(); iit!=Parent::end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=Parent::get(i).begin(); jit!=Parent::get(i).end(); jit++ ) {
	I j = jit->first;
        if ( Parent::get(i).get(j) > pOut ) pOut = Parent::get(i).get(j);
      }
    }
    return pOut;
  }

  // Scalar one-norm (sum) of matrix / vector...
  P onenorm ( ) const {
    P sum=0;
    for ( typename Parent::const_iterator iit=Parent::begin(); iit!=Parent::end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=Parent::get(i).begin(); jit!=Parent::get(i).end(); jit++ ) {
	I j = jit->first;
	sum += Parent::get(i).get(j);
      }
    }
    return sum;
  }

  //// Input / output methods...
  friend pair<StringInput,SparseMatrix<I,J,P>*> operator>> ( StringInput si, SparseMatrix<I,J,P>& m ) {
    return pair<StringInput,SparseMatrix<I,J,P>*>(si,&m);
  }
  friend StringInput operator>> ( pair<StringInput,SparseMatrix<I,J,P>*> si_m, const char* psD ) {
    if (StringInput(NULL)==si_m.first) return si_m.first;
    StringInput si; I i,j; P p;
    si=si_m.first>>i>>" : ">>j>>" = ">>p>>psD;
    if ( si!=NULL ) si_m.second->set(i).set(j) = p;
    return si;
  }
  friend ostream& operator<< ( ostream& os, const SparseMatrix<I,J,P>& m ) {
    int ctr=0;
    for ( typename Parent::const_iterator iit=m.begin(); iit!=m.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=m.get(i).begin(); jit!=m.get(i).end(); jit++ ) {
	I j = jit->first;
        os<<((0==ctr++)?"":",")<<i<<":"<<j<<"="<<m.get(i).get(j);
      }
    }
    return os;
  }
  friend String&  operator<< ( String& str, const SparseMatrix<I,J,P>& m ) {
    int ctr=0;
    for ( typename Parent::const_iterator iit=m.begin(); iit!=m.end(); iit++ ) {
      I i = iit->first;
      for ( typename SimpleHash<J,P>::const_iterator jit=m.get(i).begin(); jit!=m.get(i).end(); jit++ ) {
	I j = jit->first;
        str<<((0==ctr++)?"":",")<<i<<j;
      }
    }
    return str;
  }
};
