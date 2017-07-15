#include "ug_phrasepair.h"

namespace sapt {

void
fill_lr_vec2
( LRModel::ModelType mdl, float const* const cnt,
  float const total, float* v)
{
  if (mdl == LRModel::Monotonic)
    {
      float denom = log(total + 2);
      v[LRModel::M]  = log(cnt[LRModel::M] + 1.)      - denom;
      v[LRModel::NM] = log(total - v[LRModel::M] + 1) - denom;
    }
  else if (mdl == LRModel::LeftRight)
    {
      float denom = log(total + 2);
      v[LRModel::R] = log(cnt[LRModel::M] + cnt[LRModel::DR] + 1.) - denom;
      v[LRModel::L] = log(cnt[LRModel::S] + cnt[LRModel::DL] + 1.) - denom;
    }
  else if (mdl == LRModel::MSD)
    {
      float denom = log(total + 3);
      v[LRModel::M] = log(cnt[LRModel::M]  + 1) - denom;
      v[LRModel::S] = log(cnt[LRModel::S]  + 1) - denom;
      v[LRModel::D] = log(cnt[LRModel::DR] +
			  cnt[LRModel::DL] + 1) - denom;
    }
  else if (mdl == LRModel::MSLR)
    {
      float denom = log(total + 4);
      v[LRModel::M]  = log(cnt[LRModel::M]  + 1) - denom;
      v[LRModel::S]  = log(cnt[LRModel::S]  + 1) - denom;
      v[LRModel::DL] = log(cnt[LRModel::DL] + 1) - denom;
      v[LRModel::DR] = log(cnt[LRModel::DR] + 1) - denom;
    }
  else UTIL_THROW2("Reordering type not recognized!");
}


} // namespace sapt

