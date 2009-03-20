#pragma once

#include "math.h"
namespace Josiah {
  
  class QuenchingSchedule {
  public:
    QuenchingSchedule(float start_temp, float stop_temp, float ratio) : m_startTemp(start_temp), m_stopTemp(stop_temp), m_ratio(ratio) {}
    virtual ~QuenchingSchedule() {};
    inline float GetRatio() const { return m_ratio; }
    inline float GetStartTemp() const { return m_startTemp; }
    inline float GetStopTemp() const { return m_stopTemp; }
    virtual float GetTemperatureAtTime(int time) const = 0;
  private:
    float m_startTemp;
    float m_stopTemp;
    float m_ratio;
  };
  
  // quenches exponentially
  class ExponentialQuenchingSchedule : public QuenchingSchedule {
  public:
    ExponentialQuenchingSchedule(float start_temp, float stop_temp, float ratio); 
    
    virtual float GetTemperatureAtTime(int time) const ;
  };
  
};
