/*
 * SubPhrase.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: hieu
 */
#include "SubPhrase.h"

using namespace std;

namespace Moses2
{
SubPhrase::SubPhrase(const Phrase &origPhrase, size_t start, size_t end)
:m_origPhrase(&origPhrase)
,m_start(start)
,m_end(end)
{
}

const Word &SubPhrase::operator[](size_t pos) const
{ return (*m_origPhrase)[pos + m_start]; }

std::ostream& operator<<(std::ostream &out, const SubPhrase &obj)
{
	if (obj.GetSize()) {
		out << obj[0];
		for (size_t i = 1; i < obj.GetSize(); ++i) {
			const Word &word = obj[i];
			out << " " << word;
		}
	}
	return out;
}

SubPhrase SubPhrase::GetSubPhrase(size_t start, size_t end) const
{
	SubPhrase ret(*m_origPhrase, m_start + start, m_start + end);
	return ret;
}

}


