#pragma once
#include "PHNetState.h"

struct SCarNetUpdate
{
	u32	TimeStamp;
	std::vector<SPHNetState> StateVec;
};