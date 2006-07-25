

#include <string>
#include <iostream>
#include "SquareMatrix.h"
#include "TypeDef.h"
#include "Util.h"

using namespace std;

std::string SquareMatrix::ToString()
{
	for (size_t col = 0 ; col < m_size ; col++)
	{
		for (size_t row = 0 ; row < m_size ; row++)
			TRACE_ERR(GetScore(row, col) << " ");
		TRACE_ERR(endl);
	}
}