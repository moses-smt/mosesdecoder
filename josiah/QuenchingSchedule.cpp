#include "QuenchingSchedule.h"

namespace Josiah {
  
    ExponentialQuenchingSchedule::ExponentialQuenchingSchedule(float start_temp, float stop_temp, float ratio) : QuenchingSchedule(start_temp, stop_temp, ratio) {}
    float ExponentialQuenchingSchedule::GetTemperatureAtTime(int time) const {
      return GetStartTemp() * pow(GetRatio(), time); 
    }
}
