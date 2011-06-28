#pragma once

namespace Josiah {

class AnnealingSchedule {
 public:
  AnnealingSchedule(int length) : m_len(length) {}
  virtual ~AnnealingSchedule();
  inline int GetLength() const { return m_len; }
  virtual float GetTemperatureAtTime(int time) const = 0;
 private:
  int m_len;
};

// cools linearly
class LinearAnnealingSchedule : public AnnealingSchedule {
 public:
  LinearAnnealingSchedule(int len, float max_temp);
  virtual float GetTemperatureAtTime(int time) const;
 private:
  float starting_temp;
};

// cools exponentially
class ExponentialAnnealingSchedule : public AnnealingSchedule {
 public:
  ExponentialAnnealingSchedule(float start_temp, float stop_temp, float floor_temp, float ratio);
  virtual float GetTemperatureAtTime(int time) const;
  float GetFloorTemp() {return m_floorTemp;}
  void SetFloorTemp(float f) { m_floorTemp = f;}
 private:
  float m_startTemp;
  float m_stopTemp;
  float m_floorTemp;
  float m_ratio;
};
  
};

