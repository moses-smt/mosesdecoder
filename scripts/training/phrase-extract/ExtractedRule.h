#pragma once
/*
 *  ExtractedRule.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>

// sentence-level collection of rules
class ExtractedRule {
public:
	std::string source,target,alignment,alignmentInv,orientation,orientationForward;
	int startT,endT,startS,endS;
	float count; 
	bool allowable;
	
	ExtractedRule( int sT,int eT,int sS,int eS )
	:startT(sT)
	,endT(eT)
	,startS(sS)
	,endS(eS) 
	,source("")
	,target("")
	,alignment("")
	,alignmentInv("")
	,orientation("")
	,orientationForward("")
	,count(0)
	,allowable(true)
	{	}
	
};
