#pragma once

#include <stdint.h>

// This helper class parses a single bitcoin block
class CBlockIndex;

namespace parseblock
{

class ParseBlock
{
public:
	static ParseBlock *create(void);

	virtual uint32_t parseBlock(const CBlockIndex &index,const char *dataDir) = 0;

	virtual void release(void) = 0;
};

}
