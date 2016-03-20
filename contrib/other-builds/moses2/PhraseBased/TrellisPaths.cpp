/*
 * TrellisPaths.cpp
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */

#include "TrellisPaths.h"
#include "../legacy/Util2.h"

namespace Moses2 {

TrellisPaths::TrellisPaths() {
	// TODO Auto-generated constructor stub

}

TrellisPaths::~TrellisPaths() {
	while (!empty()) {
		TrellisPath *path = Get();
		delete path;
	}
}

void TrellisPaths::Add(TrellisPath *trellisPath)
{
  m_collection.push(trellisPath);
}

} /* namespace Moses2 */
