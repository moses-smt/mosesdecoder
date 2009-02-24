#include "AnnealingSchedule.h"

#include "StaticData.h"

namespace Moses {

AnnealingSchedule::~AnnealingSchedule() {}

LinearAnnealingSchedule::LinearAnnealingSchedule(int len, float max_temp) :
    AnnealingSchedule(len), starting_temp(max_temp) {
  cerr << "Created LinearAnnealingSchedule:\n  len=" << len << ", starting temp=" << max_temp << endl;
}

float LinearAnnealingSchedule::GetTemperatureAtTime(int time) const {
  const float temp = max(1.0f, (starting_temp -
    (static_cast<float>(time) * (starting_temp - 0.5f)) / static_cast<float>(GetLength())));
  VERBOSE(3, "Time " << time << ": temp=" << temp << endl);
  return temp;
}

}
