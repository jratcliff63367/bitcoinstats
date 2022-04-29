#include "ParseBlock.h"
#include "CBlockIndex.h"

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace parseblock
{

class ParseBlockImpl : public ParseBlock
{
public:
	ParseBlockImpl(void)
	{
	}

	virtual ~ParseBlockImpl(void)
	{
	}

	virtual uint32_t parseBlock(const CBlockIndex &index,const char *dataDir) final
	{
		uint32_t ret = 0;

		index.printInfo();

		return ret;
	}

	virtual void release(void) final
	{
		delete this;
	}
};

ParseBlock *ParseBlock::create(void)
{
	auto ret = new ParseBlockImpl;
	return static_cast< ParseBlock *>(ret);
}


}
