/*
 * Weights.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef WEIGHTS_H_
#define WEIGHTS_H_

#include <iostream>
#include "TypeDef.h"

class Weights {
	  friend std::ostream& operator<<(std::ostream &, const Weights &);
public:
	Weights();
	virtual ~Weights();

	  SCORE operator[](size_t ind) const {
		return 444.5f;
	  }

};

#endif /* WEIGHTS_H_ */
