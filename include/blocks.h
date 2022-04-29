#pragma once

#include <stdint.h>
// This class will read the LevelDB bitcoin database and
// find all information about all of the bitcoin blocks.
class CBlockIndex;

namespace blocks
{


class Blocks
{
public:
	static Blocks *create(const char *levelDBDir);

	// Return the total number of days on the blockchain
	virtual uint32_t getDayCount(void) const = 0;

	// Return the ASCII description for this day
	virtual const char *getDay(uint32_t dayIndex) const = 0; 

	virtual uint32_t getBlockHeight(void) const = 0;

	// Return the CBlockIndex structure for this block
	virtual const CBlockIndex *getBlockIndex(uint32_t blockHeight) const = 0;

	virtual void release(void) = 0;
protected:
	virtual ~Blocks(void)
	{
	}
};

}
