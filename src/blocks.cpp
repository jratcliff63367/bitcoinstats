#include "blocks.h"
#include "varint.h"
#include "KeyValueDatabase.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "CBlockIndex.h"
#include "ScopedTime.h"

#include <assert.h>
#include <unordered_map>
#include <map>

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace blocks
{

class BlockDay
{
public:
	uint32_t	mDayNumber{0};			// We assign a unique number for reach day
	uint32_t	mBlockTime{0};			// in time_t format
	uint64_t	mMinBlockHeight{0};		// the minimum block height on this day
	uint64_t	mMaxBlockHeight{0};		// the maximum block height on this day
	uint64_t	mTransactionCount{0};	// The number of transactions which occured on this day
	uint32_t	mBlockCount{0};			// the number of blocks on this day
};

using BlockIndexMap = std::unordered_map< uint32_t, CBlockIndex >;
using BlockDayMap = std::map< std::string, BlockDay >;

class BlocksImpl : public Blocks
{
public:
	BlocksImpl(const char *levelDBDir)
	{
		auto database = keyvaluedatabase::KeyValueDatabase::create(levelDBDir);
		if ( database )
		{
			bool ok = database->begin("b");
			uint32_t blockCount=0;
			printf("Scanning block index headers.\n");
			ScopedTime st("TimeSpent processing bitcoin headers");
			uint32_t blockLow=0;
			uint32_t blockHigh=0;
			bool firstBlock = true;
			while (ok )
			{
				std::string key;
				std::string value;
				ok = database->next(key,value);
				if ( ok )
				{
					CBlockIndex cb(key.c_str()+1);
					const uint8_t *start = (const uint8_t *)value.c_str();
					cb.readBlockIndex(start,value.size());

					BlockIndexMap::iterator found = mBlocks.find( uint32_t(cb.mBlockHeight));
					if ( found != mBlocks.end() )
					{
						assert(0);
					}
					else
					{
						uint32_t blockHeight = uint32_t(cb.mBlockHeight);
						mBlocks[blockHeight] = cb;
						if ( firstBlock )
						{
							blockLow = blockHeight;
							blockHigh = blockHeight;
							firstBlock = false;
						}
						else
						{
							if ( blockHeight < blockLow )
							{
								blockLow = blockHeight;
							}
							else if ( blockHeight > blockHigh )
							{
								blockHigh = blockHeight;
							}
						}
					}
					blockCount++;
				}
			}
			printf("Found %d blocks. BlockLow:%d BlockHigh:%d\n", blockCount, blockLow, blockHigh);
			assert(blockLow==0);
			for (uint32_t i=blockLow; i<=blockHigh; i++)
			{
				BlockIndexMap::iterator found = mBlocks.find(i);
				if ( found == mBlocks.end() )
				{
					printf("Unable to find block:%d\n", i);
				}
				else
				{
					CBlockIndex &cb = (*found).second;
					std::string blockTime = cb.getBlockTime();
					BlockDayMap::iterator foundDay = mDays.find(blockTime);
					if ( foundDay == mDays.end() ) // first time we have seen this day
					{
						BlockDay bd;
						bd.mBlockTime = cb.mTime;
						bd.mMinBlockHeight = cb.mBlockHeight;
						bd.mMaxBlockHeight = cb.mBlockHeight;
						bd.mTransactionCount = cb.mTransactionCount;
						bd.mBlockCount = 1;
						mDays[blockTime] = bd;
					}
					else
					{
						BlockDay &bd = (*foundDay).second;
						if ( cb.mBlockHeight < bd.mMinBlockHeight )
						{
							bd.mMinBlockHeight = cb.mBlockHeight;
						}
						if ( cb.mBlockHeight > bd.mMaxBlockHeight )
						{
							bd.mMaxBlockHeight = cb.mBlockHeight;
						}
						bd.mTransactionCount+=cb.mTransactionCount;
						bd.mBlockCount++;
						mDays[blockTime] = bd;
					}
					
				}
			}
			mDayCount = 0;
			for (auto &i:mDays)
			{
				i.second.mDayNumber = mDayCount;
				mDayCount++;
			}
			printf("%d Blocks span a total of %d days.\n",uint32_t(mBlocks.size()),  mDayCount);
			database->release();
		}
	}
	virtual ~BlocksImpl(void)
	{
	}
	virtual void release(void) final
	{
		delete this;
	}

	char getHex(uint8_t c)
	{
		char ret;
		if ( c < 10 )
		{
			ret = '0'+c;
		}
		else
		{
			ret = 'A'+(c-10);
		}
		return ret;
	}

	std::string getHex(const std::string &str)
	{
		std::string ret;
		for (auto &i:str)
		{
			uint8_t c = uint8_t(i);
			char h1 = getHex(c>>4);
			char h2 = getHex(c&15);
			ret.push_back(h1);
			ret.push_back(h2);
		}
		return ret;
	}

	std::string getHexReverse(const std::string &str)
	{
		std::string ret;
		size_t count = str.size();
		size_t index = count-1;
		for (size_t i=0; i<(count-1); i++)
		{
			uint8_t c = uint8_t(str[index]);
			char h1 = getHex(c>>4);
			char h2 = getHex(c&15);
			ret.push_back(h1);
			ret.push_back(h2);
			index--;
		}
		return ret;
	}

private:
	uint32_t		mDayCount{0}; // total number of days covered by the blockchain
	BlockIndexMap	mBlocks;
	BlockDayMap		mDays;
};

Blocks *Blocks::create(const char *levelDBDir)
{
	auto ret = new BlocksImpl(levelDBDir);
	return static_cast< Blocks *>(ret);
}


}

