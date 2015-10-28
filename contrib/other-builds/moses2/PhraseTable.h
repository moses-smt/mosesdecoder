/*
 * PhraseTable.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "TargetPhrases.h"
#include "Word.h"
#include "StatelessFeatureFunction.h"
#include "moses/Util.h"

class System;
class InputPaths;
class InputPath;

////////////////////////////////////////////////////////////////////////
class PhraseTable : public StatelessFeatureFunction
{
public:
	PhraseTable(size_t startInd, const std::string &line);
	virtual ~PhraseTable();

	virtual void SetParameter(const std::string& key, const std::string& value);
	virtual void Lookup(InputPaths &inputPaths) const;
	virtual const TargetPhrases *Lookup(InputPath &inputPath) const;

	void SetPtInd(size_t ind)
	{ m_ptInd = ind; }
	size_t GetPtInd() const
	{ return m_ptInd; }

protected:
	std::string m_path;
	size_t m_ptInd;

};

