#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "mblas/base_matrix.h"

class Weights;
class Vocab;
class Encoder;
class Decoder;

class NMT {
  public:
    NMT(const std::string& model,
        const std::string& src,
        const std::string& trg);
  
    void CalcSourceContext(const std::vector<std::string>& s);
  
  private:
    const boost::shared_ptr<Weights> w_;
    const boost::shared_ptr<Vocab> src_;
    const boost::shared_ptr<Vocab> trg_;
    
    boost::shared_ptr<Encoder> encoder_;
    boost::shared_ptr<Decoder> decoder_;
    
    boost::shared_ptr<mblas::BaseMatrix> SourceContext;
};
