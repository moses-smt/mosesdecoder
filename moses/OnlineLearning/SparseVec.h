/*
 * SparseVec.h
 *
 *  Created on: May 8, 2013
 *      Author: prashant
 */

#ifndef SPARSEVEC_H_
#define SPARSEVEC_H_

#include<vector>
#include<valarray>
#include "../Util.h"
namespace Moses {

class SparseVec {

private:
	std::vector<float> m_SparseVector;
	std::vector<float> m_coreVector;

public:
	const std::vector<float> GetSparseVector() const;
	SparseVec();
	SparseVec(const SparseVec&);
	SparseVec(int size);
	void PlusEqualsFeat(int idx, float scale);
	void MinusEqualsFeat(int idx, float scale);
	void MultiplyEqualsFeat(int idx, float scale);
	void Assign(int idx, float val);
	int AddFeat(float val);
	void AddFeat(int idx, float val);
	void MultiplyEquals(std::vector<int>& IdxVec, std::vector<float>& Val);
	void MultiplyEquals(const float& val);
	void PlusEquals(std::vector<int>& IdxVec, std::vector<float>& Val);
	void PlusEquals(SparseVec& sv);
	void MinusEquals(std::vector<int>& IdxVec, std::vector<float>& Val);
	const size_t GetSize() const;
	const float getElement(const int idx) const;
	void coreAssign(std::valarray<float>& x);
	float GetL1Norm();
	SparseVec& operator*= (const float& rhs);

};

} /* namespace SparseVector */
#endif /* SPARSEVEC_H_ */
