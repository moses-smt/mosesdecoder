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

#ifndef _NL_PROB__
#define _NL_PROB__

#include "nl-safeids.h"
#include <math.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////////

typedef double PDFVal;
typedef double LogPDFVal;

////////////////////////////////////////////////////////////////////////////////

class Prob {
   private:
      double gVal;

   public:
      Prob ( )              { gVal = 0.0; }
      Prob (double d)       { gVal = d;   }
      Prob (const char* ps) { gVal = atof(ps); }
      
      operator double() const { return gVal; }
      double toDouble() const { return gVal; }
      Prob& operator+= ( const Prob p ) { gVal += p.gVal; return *this; }
      Prob& operator-= ( const Prob p ) { gVal -= p.gVal; return *this; }
      Prob& operator*= ( const Prob p ) { gVal *= p.gVal; return *this; }
      Prob& operator/= ( const Prob p ) { gVal /= p.gVal; return *this; }

      friend ostream& operator<< ( ostream& os, const Prob& pr ) { return os<<pr.toDouble(); }
      friend String&  operator<< ( String& str, const Prob& pr ) { return str<<pr.toDouble(); }
      friend pair<StringInput,Prob*> operator>> ( StringInput si, Prob& n ) { return pair<StringInput,Prob*>(si,&n); }
      friend StringInput             operator>> ( pair<StringInput,Prob*> si_n, const char* psDlm ) { 
        double d=0.0; StringInput si=si_n.first>>d>>psDlm; *si_n.second=Prob(d); return si; }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  LogProb -- encapsulate min probability in sum operations
//
////////////////////////////////////////////////////////////////////////////////

//#define MIN_LOG_PROB (1-MAXINT)
#define MIN_LOG_PROB INT_MIN

class LogProb : public Id<int> {
 public:
  // Constructor / destructor methods...
  LogProb ( )          { set(MIN_LOG_PROB); }
  LogProb ( int i )    { set(i); }
  LogProb ( double d ) { set(int(100.0*log(d))); }
  LogProb ( Prob   d ) { set(int(100.0*log(d))); }
  // Specification methods...
  LogProb& operator+= ( const LogProb i ) { assert(false); return *this; }
  LogProb& operator*= ( const LogProb i )
    {   if((toInt() == MIN_LOG_PROB) || (i.toInt() == MIN_LOG_PROB)){
          set(MIN_LOG_PROB);
        }else{
          // Correct underflow if result is greater than either addend...
          int k=toInt()+i.toInt(); set((toInt()<=0 && i.toInt()<=0 && (k>i.toInt() || k>toInt())) ? MIN_LOG_PROB : k);
        }
        return *this;
    }
  LogProb& operator-= ( const LogProb i ) { assert(false); return *this; }
  LogProb& operator/= ( const LogProb i )
    {   if((toInt() == MIN_LOG_PROB) || (i.toInt() == MIN_LOG_PROB)){
            set(MIN_LOG_PROB);
        }else{
          int k=toInt()-i.toInt(); set(k);
        }
        return *this;
    }

  // Extraction methods...
  bool    operator==( const LogProb i ) const { return(i.toInt()==toInt()); }
  bool    operator!=( const LogProb i ) const { return(i.toInt()!=toInt()); }
  LogProb operator+ ( const LogProb i ) const { assert(false); return *this; }   // no support for addition in log mode!
  LogProb operator- ( const LogProb i ) const { assert(false); return *this; }   // no support for addition in log mode!
  LogProb operator* ( const LogProb i ) const {
    int k;
    if((toInt() == MIN_LOG_PROB) || (i.toInt() == MIN_LOG_PROB)){
      k = MIN_LOG_PROB;
    }else{
      k=toInt()+i.toInt();
      // Correct underflow if result is greater than either addend...
      k = (toInt()<0 && i.toInt()<0 && (k>i.toInt() || k>toInt())) ? MIN_LOG_PROB : k;
    }
    return LogProb(k);
  }
  LogProb operator/ ( const LogProb i ) const {
    int k;
    if((toInt() == MIN_LOG_PROB) || (i.toInt() == MIN_LOG_PROB)){
      k = MIN_LOG_PROB;
    }else{
      k = toInt()-i.toInt();
      // // Correct underflow if result is greater than either addend...
      // k = (toInt()<0 && -i.toInt()<0 && (k>-i.toInt() || k>toInt())) ? MIN_LOG_PROB : k;
    }
    return LogProb(k);
  }
  Prob   toProb()   const { return exp(double(toInt())/100.0);  }
  double toDouble() const { return toProb().toDouble(); }
//  operator double() const { return exp(toInt()/100.0); }
  friend ostream& operator<< ( ostream& os, const LogProb& lp ) { return os<<lp.toInt(); }
  friend String&  operator<< ( String& str, const LogProb& lp ) { return str<<lp.toInt(); }
  friend pair<StringInput,LogProb*> operator>> ( StringInput si, LogProb& n ) { return pair<StringInput,LogProb*>(si,&n); }
  friend StringInput                operator>> ( pair<StringInput,LogProb*> si_n, const char* psDlm ) { 
    double d=0.0; StringInput si=si_n.first>>d>>psDlm; *si_n.second=LogProb(d); return si; }
};

#endif /* _NL_PROB__ */
