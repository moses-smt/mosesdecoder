/*
 * InputType.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#ifndef INPUTTYPE_H_
#define INPUTTYPE_H_

namespace Moses2 {

class InputType {
public:
	InputType(long translationId)
	:m_translationId(translationId)
    {}

	virtual ~InputType();

	  long GetTranslationId() const {
		return m_translationId;
	  }

protected:
  long m_translationId; 	//< contiguous Id


};

} /* namespace Moses2 */

#endif /* INPUTTYPE_H_ */
