#pragma once

namespace Moses {

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

};

