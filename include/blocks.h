#pragma once

// This class will read the LevelDB bitcoin database and
// find all information about all of the bitcoin blocks.

namespace blocks
{

class Blocks
{
public:
	static Blocks *create(const char *levelDBDir);
	virtual void release(void) = 0;
protected:
	virtual ~Blocks(void)
	{
	}
};

}
