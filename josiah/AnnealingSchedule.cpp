#include "AnnealingSchedule.h"
#include "StaticData.h"

using namespace Moses;
using namespace std;

namespace Josiah {

AnnealingSchedule::~AnnealingSchedule() {}

LinearAnnealingSchedule::LinearAnnealingSchedule(int len, float max_temp) :
    AnnealingSchedule(len), starting_temp(max_temp) {
  VERBOSE(2, "Created LinearAnnealingSchedule:\n  len=" << len << ", starting temp=" << max_temp << endl);
}

float LinearAnnealingSchedule::GetTemperatureAtTime(int time) const {
  const float temp = max(1.0f, (starting_temp -
    (static_cast<float>(time) * (starting_temp - 0.5f)) / static_cast<float>(GetLength())));
  VERBOSE(3, "Time " << time << ": temp=" << temp << endl);
  return temp;
}

ExponentialAnnealingSchedule::ExponentialAnnealingSchedule(float start_temp, float stop_temp, float floor_temp, float ratio) :
  AnnealingSchedule(0), m_startTemp(start_temp), m_stopTemp(stop_temp), m_floorTemp(floor_temp) , m_ratio (ratio){
    VERBOSE(2, "Created ExponentialAnnealingSchedule:\n  starting temp=" << start_temp << ", stopping temp=" << stop_temp << ", floor temp=" << m_floorTemp << ", ratio: " << ratio << endl);
}

float ExponentialAnnealingSchedule::GetTemperatureAtTime(int time) const {
  float curTemp = m_startTemp * pow(m_ratio, time);
  if (curTemp < m_stopTemp)
    return m_floorTemp;
  return curTemp;
}
  
  
}
