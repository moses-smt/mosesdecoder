#pragma once
#include <valarray>
#include <vector>
#include <iostream>

namespace Josiah {
  
class SufficientStats {
  public:
    virtual ~SufficientStats() {}
};
  
class BleuSufficientStats : public SufficientStats {  
  public: 
    std::valarray<float> hyp;
    std::valarray<float> correct;
    float hyp_len;
    float ref_len;
    float src_len;
    BleuSufficientStats(int n = 4);
    BleuSufficientStats(std::vector<float> & data) { initialise(data); }
    BleuSufficientStats& operator+= (const BleuSufficientStats &other );
    BleuSufficientStats operator/ ( float num );
    BleuSufficientStats& operator*= (float  );
    std::vector<float> data(); 
    virtual ~BleuSufficientStats() {}
    friend std::ostream& operator<<(std::ostream& out, const BleuSufficientStats& stats);
    void Zero();
    void initialise( std::vector<float> & data);
};  

class BleuDefaultSmoothingSufficientStats : public BleuSufficientStats {  
  public: 
    BleuDefaultSmoothingSufficientStats(int n = 4, float smoothing_factor = 0.01);
    virtual ~BleuDefaultSmoothingSufficientStats() {}
};  
  
  
}
